/*
** Zabbix
** Copyright (C) 2001-2022 Zabbix SIA
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**/

#include "zbxconnector.h"

#include "log.h"
#include "zbxipcservice.h"
#include "zbxserialize.h"

void	zbx_connector_send(zbx_uint32_t code, unsigned char *data, zbx_uint32_t size)
{
	static zbx_ipc_socket_t	socket;

	/* each process has a permanent connection to availability manager */
	if (0 == socket.fd)
	{
		char	*error = NULL;

		if (FAIL == zbx_ipc_socket_open(&socket, ZBX_IPC_SERVICE_CONNECTOR, SEC_PER_MIN, &error))
		{
			zabbix_log(LOG_LEVEL_CRIT, "cannot connect to availability manager service: %s", error);
			exit(EXIT_FAILURE);
		}
	}

	if (FAIL == zbx_ipc_socket_write(&socket, code, data, size))
	{
		zabbix_log(LOG_LEVEL_CRIT, "cannot send data to availability manager service");
		exit(EXIT_FAILURE);
	}
}

zbx_uint32_t	zbx_connector_pack_diag_stats(unsigned char **data, zbx_uint64_t queued)
{
	unsigned char	*ptr;
	zbx_uint32_t	data_len = 0;

	zbx_serialize_prepare_value(data_len, queued);

	*data = (unsigned char *)zbx_malloc(NULL, data_len);

	ptr = *data;
	(void)zbx_serialize_value(ptr, queued);

	return data_len;
}

/******************************************************************************
 *                                                                            *
 * Purpose: unpack preprocessing test data from IPC data buffer               *
 *                                                                            *
 * Parameters: total      - [OUT] the number of values                        *
 *             queued     - [OUT] the number of values waiting to be          *
 *                                preprocessed                                *
 *             processing - [OUT] the number of values being preprocessed     *
 *             done       - [OUT] the number of values waiting to be flushed  *
 *                                that are either preprocessed or did not     *
 *                                require preprocessing                       *
 *             pending    - [OUT] the number of values pending to be          *
 *                                preprocessed after previous value for       *
 *                                example delta, throttling depends on        *
 *                                previous value                              *
 *             data       - [IN] IPC data buffer                              *
 *                                                                            *
 ******************************************************************************/
static void	zbx_connector_unpack_diag_stats(zbx_uint64_t *queued, const unsigned char *data)
{
	(void)zbx_deserialize_int(data, queued);
}

/******************************************************************************
 *                                                                            *
 * Purpose: get preprocessing manager diagnostic statistics                   *
 *                                                                            *
 ******************************************************************************/
int	zbx_connector_get_diag_stats(zbx_uint64_t *queued, char **error)
{
	unsigned char	*result;

	if (SUCCEED != zbx_ipc_async_exchange(ZBX_IPC_SERVICE_CONNECTOR, ZBX_IPC_CONNECTOR_DIAG_STATS,
			SEC_PER_MIN, NULL, 0, &result, error))
	{
		return FAIL;
	}

	zbx_connector_unpack_diag_stats(queued, result);
	zbx_free(result);

	return SUCCEED;
}

int	zbx_connector_get_queue_size(zbx_uint64_t *size, char **error)
{
	zbx_ipc_message_t	message;
	zbx_ipc_socket_t	connector_socket;
	int			ret = FAIL;

	if (FAIL == zbx_ipc_socket_open(&connector_socket, ZBX_IPC_SERVICE_CONNECTOR, SEC_PER_MIN, error))
		return FAIL;

	zbx_ipc_message_init(&message);

	if (FAIL == zbx_ipc_socket_write(&connector_socket, ZBX_IPC_CONNECTOR_QUEUE, NULL, 0))
	{
		*error = zbx_strdup(NULL, "cannot send queue request to connector service");
		goto out;
	}

	if (FAIL == zbx_ipc_socket_read(&connector_socket, &message))
	{
		*error = zbx_strdup(NULL, "cannot read queue response from connector service");
		goto out;
	}

	memcpy(size, message.data, sizeof(zbx_uint64_t));
	ret = SUCCEED;
out:
	zbx_ipc_socket_close(&connector_socket);
	zbx_ipc_message_clean(&message);

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: pack top request data into a single buffer that can be used in IPC*
 *                                                                            *
 * Parameters: data  - [OUT] memory buffer for packed data                    *
 *             field - [IN] the sort field                                    *
 *             limit - [IN] the number of top values to return                *
 *                                                                            *
 ******************************************************************************/
static zbx_uint32_t	connector_pack_top_items_request(unsigned char **data, int limit)
{
	zbx_uint32_t	data_len = 0;

	zbx_serialize_prepare_value(data_len, limit);
	*data = (unsigned char *)zbx_malloc(NULL, data_len);
	(void)zbx_serialize_value(*data, limit);

	return data_len;
}

/******************************************************************************
 *                                                                            *
 * Purpose: unpack preprocessing test data from IPC data buffer               *
 *                                                                            *
 * Parameters: data  - [OUT] memory buffer for packed data                    *
 *             limit - [IN] the number of top values to return                *
 *                                                                            *
 ******************************************************************************/
void	zbx_connector_unpack_top_request(int *limit, const unsigned char *data)
{
	(void)zbx_deserialize_value(data, limit);
}

/******************************************************************************
 *                                                                            *
 * Purpose: unpack preprocessing test data from IPC data buffer               *
 *                                                                            *
 * Parameters: items - [OUT] the item diag data                               *
 *             data  - [IN] memory buffer for packed data                     *
 *                                                                            *
 ******************************************************************************/
static void	connector_unpack_top_result(zbx_vector_ptr_t *connector_stats, const unsigned char *data)
{
	int	i, connectors_num;

	data += zbx_deserialize_value(data, &connectors_num);

	if (0 != connectors_num)
	{
		zbx_vector_ptr_reserve(connector_stats, (size_t)connectors_num);

		for (i = 0; i < connectors_num; i++)
		{
			zbx_connector_stat_t	*connector_stat;

			connector_stat = (zbx_connector_stat_t *)zbx_malloc(NULL, sizeof(zbx_connector_stat_t));
			data += zbx_deserialize_value(data, &connector_stat->connectorid);
			data += zbx_deserialize_value(data, &connector_stat->values_num);
			data += zbx_deserialize_value(data, &connector_stat->links_num);
			data += zbx_deserialize_value(data, &connector_stat->queued_links_num);
			zbx_vector_ptr_append(connector_stats, connector_stat);
		}
	}
}

/******************************************************************************
 *                                                                            *
 * Purpose: get the top N items by the number of queued values                *
 *                                                                            *
 ******************************************************************************/
static int	connector_get_top_items(int limit, zbx_vector_ptr_t *items, char **error, zbx_uint32_t code)
{
	int		ret;
	unsigned char	*data, *result;
	zbx_uint32_t	data_len;

	data_len = connector_pack_top_items_request(&data, limit);

	if (SUCCEED != (ret = zbx_ipc_async_exchange(ZBX_IPC_SERVICE_CONNECTOR, code, SEC_PER_MIN, data, data_len,
			&result, error)))
	{
		goto out;
	}

	connector_unpack_top_result(items, result);
	zbx_free(result);
out:
	zbx_free(data);

	return ret;
}

/******************************************************************************
 *                                                                            *
 * Purpose: get the top N items by the number of queued values                *
 *                                                                            *
 ******************************************************************************/
int	zbx_connector_get_top_connectors(int limit, zbx_vector_ptr_t *items, char **error)
{
	return connector_get_top_items(limit, items, error, ZBX_IPC_CONNECTOR_TOP_CONNECTORS);
}

zbx_uint32_t	zbx_connector_pack_top_connectors_result(unsigned char **data, zbx_connector_stat_t **connector_stats,
		int connector_stats_num)
{
	unsigned char	*ptr;
	zbx_uint32_t	data_len = 0, item_len = 0;
	int		i;

	if (0 != connector_stats_num)
	{
		zbx_serialize_prepare_value(item_len, connector_stats[0]->connectorid);
		zbx_serialize_prepare_value(item_len, connector_stats[0]->values_num);
		zbx_serialize_prepare_value(item_len, connector_stats[0]->links_num);
		zbx_serialize_prepare_value(item_len, connector_stats[0]->queued_links_num);
	}

	zbx_serialize_prepare_value(data_len, connector_stats_num);
	data_len += item_len * (zbx_uint32_t)connector_stats_num;
	*data = (unsigned char *)zbx_malloc(NULL, data_len);

	ptr = *data;
	ptr += zbx_serialize_value(ptr, connector_stats_num);

	for (i = 0; i < connector_stats_num; i++)
	{
		ptr += zbx_serialize_value(ptr, connector_stats[i]->connectorid);
		ptr += zbx_serialize_value(ptr, connector_stats[i]->values_num);
		ptr += zbx_serialize_value(ptr, connector_stats[i]->links_num);
		ptr += zbx_serialize_value(ptr, connector_stats[i]->queued_links_num);
	}

	return data_len;
}

/******************************************************************************
 *                                                                            *
 * Purpose: frees interface availability data                                 *
 *                                                                            *
 * Parameters: availability - [IN] interface availability data                *
 *                                                                            *
 ******************************************************************************/
void	zbx_connector_object_free(zbx_connector_object_t connector_object)
{
	zbx_vector_uint64_destroy(&connector_object.ids);
	zbx_free(connector_object.str);
}

/******************************************************************************
 *                                                                            *
 * Purpose: frees interface availability data                                 *
 *                                                                            *
 * Parameters: availability - [IN] interface availability data                *
 *                                                                            *
 ******************************************************************************/
void	zbx_connector_data_point_free(zbx_connector_data_point_t connector_data_point)
{
	zbx_free(connector_data_point.str);
}

ZBX_PTR_VECTOR_IMPL(connector_object, zbx_connector_object_t)
ZBX_PTR_VECTOR_IMPL(connector_data_point, zbx_connector_data_point_t)

