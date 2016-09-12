
#include "Log.h"

#include <assert.h>

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include "mem_alloc.h"
#include "dbregistry.h"

namespace mocainput {

	DBList::~DBList() {
		LOGD("DBRegistry::~DBList()");

		for (int i = 0; i < m_db_count; i++) {
			FREE(m_dbs[i].path);
		}
	}

	bool DBList::add_db(int id, char* path)	{
		LOGD("DBRegistry::add_db(0x%X, %s)...", id, path);

		if (m_db_count >= MAX_DBS) {
			return false;
		}

		char* newpath = (char*)MALLOC(1+strlen(path));
		if (!newpath) {
			return false;
		}

		strcpy(newpath, path);

		m_dbs[m_db_count].id = id;
		m_dbs[m_db_count].path = newpath;

		m_db_count++;

		LOGD("DBRegistry::add_db(0x%X, %s)...count = %d", id, path, m_db_count);

		return true;
	}

	char* DBList::get_path(int id) const {
		int i;
		for (i = 0; i < m_db_count; i++) {
			if (m_dbs[i].id == id) {
				return m_dbs[i].path;
			}
		}
		return (char*)NULL;
	}

	DBRegistry::DBRegistry(const char* fn) {

	    LOGD("DBRegistry::DBRegistry(%s)", fn == NULL ? sDatabaseConfiFile : fn);

	    read_conf_file(fn == NULL ? sDatabaseConfiFile : fn);
	}

	DBRegistry::~DBRegistry() {

	}

	char* DBRegistry::get_kdb_path(int id) const {
		char* file = m_kdbs.get_path(id);
		LOGD("DBRegistry::get_kdb_path(0x%X)...%s", id, file);
		return file;
	}
	bool DBRegistry::isThereKdbPathFor(int id) const {
	    return m_kdbs.get_path(id) != 0;
	}

	char* DBRegistry::get_ldb_path(int id) const {
		char* file =  m_ldbs.get_path(id);
		LOGD("DBRegistry::get_ldb_path(0x%X)...%s", id, file);
		return file;
	}

    char* DBRegistry::get_udb_path(int id) const {
        char* file =  m_udbs.get_path(id);
        LOGD("DBRegistry::get_udb_path(0x%X)...%s", id, file);
        return file;
    }

    char* DBRegistry::get_asdb_path(int id) const {
        char* file =  m_asdbs.get_path(id);
        LOGD("DBRegistry::get_asdb_path(0x%X)...%s", id, file);
        return file;
    }

    char* DBRegistry::get_cdb_path(int id) const {
        char* file =  m_cdbs.get_path(id);
        LOGD("DBRegistry::get_cdb_path(0x%X)...%s", id, file);
        return file;
    }

    char* DBRegistry::get_mdb_path(int id) const {
        char* file =  m_mdbs.get_path(id);
        LOGD("DBRegistry::get_mdb_path(0x%X)...%s", id, file);
        return file;
    }

    char* DBRegistry::get_hwr_dic_path(int id) const {
        char* file =  m_hwr_dic.get_path(id);
        LOGD("DBRegistry::get_hwr_dic_path(0x%X)...%s", id, file);
        return file;
    }

    char* DBRegistry::get_hwr_db_template_path(int id) const {
        char* file =  m_hwr_db_template.get_path(id);
        LOGD("DBRegistry::get_hwr_db_template_path(0x%X)...%s", id, file);
        return file;
    }

    bool DBRegistry::_is_white_space(char c) {
		return c == ' ' || c == '\t';
	}

	void DBRegistry::read_conf_file(const char* fn) {

		LOGD("DBRegistry::read_conf_file(%s)...", fn);

		if (!fn) {
			return;
		}

		char linebuf[max_line_len], *line;
		int id;
		char path[max_line_len];

		eState state = STATE_NONE;

		FILE* f = fopen(fn, "r");
		if (!f) {
			LOGE("DBRegistry::read_conf_file(%s)...open - error(%s)", fn, strerror(errno));
			return;
		}

		while (fgets(linebuf, max_line_len, f)) {
			line = linebuf;

			while (*line && _is_white_space(line[0])) {
				line++;
			}

			// ignore comment or empty lines
			if (line[0] == '#' || !*line) {
				continue;
			}

			if (!strncmp(line, "[kdb]", 5)) {
				state = STATE_KDB;
				LOGD("DBRegistry::read_conf_file()...kdb state");
				continue;
			}

			if (!strncmp(line, "[ldb]", 5)) {
				state = STATE_LDB;
				LOGD("DBRegistry::read_conf_file()...ldb state");
				continue;
			}

            if (!strncmp(line, "[udb]", 5)) {
                state = STATE_UDB;
                LOGD("DBRegistry::read_conf_file()...udb state");
                continue;
            }

            if (!strncmp(line, "[asdb]", 6)) {
                state = STATE_ASDB;
                LOGD("DBRegistry::read_conf_file()...asdb state");
                continue;
            }

            if (!strncmp(line, "[cdb]", 5)) {
                state = STATE_CDB;
                LOGD("DBRegistry::read_conf_file()...cdb state");
                continue;
            }

            if (!strncmp(line, "[mdb]", 5)) {
                state = STATE_MDB;
                LOGD("DBRegistry::read_conf_file()...mdb state");
                continue;
            }

            if (!strncmp(line, "[hwr_dic]", 9)) {
                state = STATE_HWR_DIC;
                LOGD("DBRegistry::read_conf_file()...hwr_dic state");
                continue;
            }

            if (!strncmp(line, "[hwr_db_template]", 17)) {
                state = STATE_HWR_DB_TEMPLATE;
                LOGD("DBRegistry::read_conf_file()...hwr_db_template state");
                continue;
            }

			if (2 == sscanf(line, "%i %s", &id, path)) {
				if (STATE_KDB == state) {
					if (!m_kdbs.add_db(id, path)) {
						LOGE("DBRegistry::read_conf_file(%s)...m_kdbs.add_db - error", fn);
					}
				}
				else if(STATE_LDB == state) {
					if (!m_ldbs.add_db(id, path)) {
						LOGE("DBRegistry::read_conf_file(%s)...m_ldbs.add_db - error", fn);
					}
				}
                else if(STATE_UDB == state) {
                    if (!m_udbs.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_udbs.add_db - error", fn);
                    }
                }
                else if(STATE_ASDB == state) {
                    if (!m_asdbs.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_asdbs.add_db - error", fn);
                    }
                }
                else if(STATE_CDB == state) {
                    if (!m_cdbs.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_cdbs.add_db - error", fn);
                    }
                }
                else if(STATE_MDB == state) {
                    if (!m_mdbs.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_mdbs.add_db - error", fn);
                    }
                }
                else if(STATE_HWR_DIC == state) {
                    if (!m_hwr_dic.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_hwr_dic.add_db - error", fn);
                    }
                }
                else if(STATE_HWR_DB_TEMPLATE == state) {
                    if (!m_hwr_db_template.add_db(id, path)) {
                        LOGE("DBRegistry::read_conf_file(%s)...m_hwr_db_template.add_db - error", fn);
                    }
                }
				else {
					LOGE("DBRegistry::read_conf_file(%s)...sscanf - error", fn);
				}
			}
		}

		fclose(f);
		LOGD("DBRegistry::read_conf_file(%s)...ok - found %d(ldb), %d (kdb)", fn, m_ldbs.m_db_count, m_kdbs.m_db_count);
	}

	// static
	int DBRegistry::instanceCount = 0;
	DBRegistry* DBRegistry::singleDBRegistryInstance = 0;
	DBRegistry* DBRegistry::getInstance(const char* fn)
	{
		if (DBRegistry::singleDBRegistryInstance == 0) {
			DBRegistry::singleDBRegistryInstance = new DBRegistry(fn);
		}

		++DBRegistry::instanceCount;

		LOGD("DBRegistry::getInstance() refCount = %d, file = %s", DBRegistry::instanceCount, fn == 0 ? "null" : fn);

		return DBRegistry::singleDBRegistryInstance;
	}

	void DBRegistry::deleteInstance()
	{
		if (DBRegistry::instanceCount > 0) {
			--DBRegistry::instanceCount;
		}

		LOGD("DBRegistry::deleteInstance() refCount = %d", DBRegistry::instanceCount);

		if (DBRegistry::singleDBRegistryInstance != 0 && DBRegistry::instanceCount == 0) {
			delete DBRegistry::singleDBRegistryInstance;
			DBRegistry::singleDBRegistryInstance = 0;
		}
	}
}
