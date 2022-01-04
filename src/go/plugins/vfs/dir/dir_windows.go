/*
** Zabbix
** Copyright (C) 2001-2021 Zabbix SIA
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

package dir

import (
	"io/fs"
	"os"
	"syscall"
)

func (cp *common) osSkip(path string, d fs.DirEntry) bool {

	i, err := d.Info()
	if err != nil {
		impl.Logger.Errf("failed to get file info for path %s, %s", path, err.Error())
		return true
	}

	if attr, ok := i.Sys().(*syscall.Win32FileAttributeData); ok {
		if attr.FileAttributes&syscall.FILE_ATTRIBUTE_REPARSE_POINT != 0 &&
			attr.FileAttributes&syscall.FILE_ATTRIBUTE_DIRECTORY != 0 {
			return true
		}
	} else {
		impl.Logger.Errf("failed to get system file attribute data")
		return true
	}

	for _, f := range cp.files {
		if os.SameFile(f, i) {
			return true
		}
	}

	cp.files = append(cp.files, i)

	return false
}
