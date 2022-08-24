<?php
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


require_once dirname(__FILE__).'/testAuditlogCommon.php';

/**
 * @backup  actions, ids
 */
class testAuditlogAction extends testAuditlogCommon {

	public $resourceid;

	public function testAuditlogAction_Create() {


		$create = $this->call('action.create', [
			[
				'name' => 'Audit action',
				'eventsource' => 0,
				'status' => 0,
				'esc_period' => '2m',
				'filter' => [
					'evaltype' => 0,
					'conditions' => [
						[
							'conditiontype' => 1,
							'operator' => 0,
							'value' => 10084
						]
					]
				],
				'operations' => [
					[
						'operationtype' => 0,
						'esc_period' => '0s',
						'esc_step_from' => 1,
						'esc_step_to' => 2,
						'evaltype' => 0,
						'opmessage_grp' => [
							[
								'usrgrpid' => 7
							]
						],
						'opmessage' => [
							'default_msg' => 1,
							'mediatypeid' => 1
						]
					]
				],
				'recovery_operations' => [
					[
						'operationtype' => 11,
						'opmessage' => [
							'default_msg' => 1
						]
					]
				],
				'update_operations' => [
					[
						'operationtype' => 12,
						'opmessage' => [
							'default_msg' => 0,
							'message' => 'Custom update operation message body',
							'subject' => 'Custom update operation message subject'
						]
					]
				],
				'pause_suppressed' => 0,
				'notify_if_canceled' => 0
			]
		]);
		$this->resourceid = $create['result']['actionids'][0];

		$operationid = CDBHelper::getAll('SELECT operationid FROM operations WHERE actionid='.$this->resourceid.' AND operationtype In (0,11,12)');
		$conditiodid = CDBHelper::getAll('SELECT conditionid FROM conditions WHERE actionid='.$this->resourceid);
		$op_group = CDBHelper::getAll('SELECT opmessage_grpid FROM opmessage_grp WHERE operationid='.$operationid[0]['operationid']);

		$created = "{\"action.name\":[\"add\",\"Audit action\"],\"action.esc_period\":[\"add\",\"2m\"],".
			"\"action.filter\":[\"add\"],".
			"\"action.filter.conditions[".$conditiodid[0]['conditionid']."]\":[\"add\"],".
			"\"action.filter.conditions[".$conditiodid[0]['conditionid']."].conditiontype\":[\"add\",\"1\"],".
			"\"action.filter.conditions[".$conditiodid[0]['conditionid']."].value\":[\"add\",\"10084\"],".
			"\"action.filter.conditions[".$conditiodid[0]['conditionid']."].conditionid\":[\"add\",\"".$conditiodid[0]['conditionid']."\"],".
			"\"action.operations[".$operationid[0]['operationid']."]\":[\"add\"],".
			"\"action.operations[".$operationid[0]['operationid']."].esc_period\":[\"add\",\"0s\"],".
			"\"action.operations[".$operationid[0]['operationid']."].esc_step_to\":[\"add\",\"2\"],".
			"\"action.operations[".$operationid[0]['operationid']."].opmessage_grp[".$op_group[0]['opmessage_grpid']
			."]\":[\"add\"],".
			"\"action.operations[".$operationid[0]['operationid']."].opmessage_grp[".$op_group[0]['opmessage_grpid'].
			"].usrgrpid\":[\"add\",\"7\"],".
			"\"action.operations[".$operationid[0]['operationid']."].opmessage_grp[".$op_group[0]['opmessage_grpid'].
			"].opmessage_grpid\":[\"add\",\"".$op_group[0]['opmessage_grpid']."\"],".
			"\"action.operations[".$operationid[0]['operationid']."].opmessage\":[\"add\"],".
			"\"action.operations[".$operationid[0]['operationid']."].opmessage.mediatypeid\":[\"add\",\"1\"],".
			"\"action.operations[".$operationid[0]['operationid']."].operationid\":[\"add\",\"".$operationid[0]['operationid']."\"],".
			"\"action.recovery_operations[".$operationid[1]['operationid']."]\":[\"add\"],".
			"\"action.recovery_operations[".$operationid[1]['operationid']."].operationtype\":[\"add\",\"11\"],".
			"\"action.recovery_operations[".$operationid[1]['operationid']."].opmessage\":[\"add\"],".
			"\"action.recovery_operations[".$operationid[1]['operationid']."].recovery\":[\"add\",\"1\"],".
			"\"action.recovery_operations[".$operationid[1]['operationid']."].operationid\":[\"add\",\"".$operationid[1]['operationid']."\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."]\":[\"add\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].operationtype\":[\"add\",\"12\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].opmessage\":[\"add\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].opmessage.default_msg\":[\"add\",\"0\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].opmessage.message\":[\"add\",\"Custom update operation message body\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].opmessage.subject\":[\"add\",\"Custom update operation message subject\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].recovery\":[\"add\",\"2\"],".
			"\"action.update_operations[".$operationid[2]['operationid']."].operationid\":[\"add\",\"".$operationid[2]['operationid']."\"],".
			"\"action.pause_suppressed\":[\"add\",\"0\"],".
			"\"action.notify_if_canceled\":[\"add\",\"0\"],".
			"\"action.actionid\":[\"add\",\"$this->resourceid\"]}";

		$this->sendGetRequest('details', 0, $created);
	}

	public function testAuditlogAction_Update() {
		$updated = "{\"action.operations[3].opmessage_grp[1]\":[\"delete\"],\"action.filter.conditions[99]\":[".
				"\"add\"],\"action.operations[3].opmessage_grp[98]\":[\"add\"],\"action.name\":[\"update\",".
				"\"Updated action audit\",\"Report problems to Zabbix administrators\"],\"action.esc_period".
				"\":[\"update\",\"15m\",\"1h\"],\"action.filter\":[\"update\"],\"action.filter.evaltype\":[".
				"\"update\",\"2\",\"0\"],\"action.filter.conditions[99].conditiontype\":[\"add\",\"3\"],".
				"\"action.filter.conditions[99].operator\":[\"add\",\"2\"],\"action.filter.conditions[99].value".
				"\":[\"add\",\"Trigger name\"],\"action.filter.conditions[99].conditionid\":[\"add\",\"99".
				"\"],\"action.operations[3]\":[\"update\"],\"action.operations[3].esc_period\":[\"update\",".
				"\"1000\",\"0\"],\"action.operations[3].esc_step_to\":[\"update\",\"2\",\"1\"],".
				"\"action.operations[3].evaltype\":[\"update\",\"1\",\"0\"],".
				"\"action.operations[3].opmessage_grp[98].usrgrpid\":[\"add\",\"9\"],".
				"\"action.operations[3].opmessage_grp[98].opmessage_grpid\":[\"add\",\"98\"],".
				"\"action.operations[3].opmessage\":[\"update\"],\"action.operations[3].opmessage.default_msg".
				"\":[\"update\",\"0\",\"1\"],\"action.operations[3].opmessage.message\":[\"update\",".
				"\"Updated audit message\",\"\"],\"action.operations[3].opmessage.subject\":[\"update\",".
				"\"Updated audit message\",\"\"]}";

		$this->call('action.update', [
			[
				'actionid' => 3,
				'name' => 'Updated action audit',
				'status' => 1,
				'esc_period' => '15m',
				'filter' => [
					'evaltype' => 2,
					'conditions' => [
						[
							'conditiontype' => 3,
							'operator' => 2,
							'value' => 'Trigger name'
						]
					]
				],
				'operations' => [
					[
						'operationtype' => 0,
						'esc_period' => 1000,
						'esc_step_from' => 1,
						'esc_step_to' => 2,
						'evaltype' => 1,
						'opmessage_grp' => [
							[
								'usrgrpid' => 9
							]
						],
						'opmessage' => [
							'default_msg' => 0,
							'message' => 'Updated audit message',
							'subject' => 'Updated audit message'
						]
					]
				],
				'pause_suppressed' => 1,
				'notify_if_canceled' => 1
			]
		]);

		$this->sendGetRequest('details', 1, $updated);
	}

	public function testAuditlogAction_Delete() {
		$this->call('action.delete', [3]);
		$this->sendGetRequest('resourcename', 2, 'Updated action audit');
	}
}
