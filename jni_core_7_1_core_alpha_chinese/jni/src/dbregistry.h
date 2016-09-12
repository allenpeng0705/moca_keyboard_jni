#ifndef __DBREGISTRY_H
#define __DBREGISTRY_H

#define MAX_DBS 32

// TODO: Change so we can build for device or test project
#ifdef _TEST_CODE_
static const char* sDatabaseConfiFile = "databases.conf";
#else
static const char* sDatabaseConfiFile = "/system/usr/moca/config/databases.conf";
#endif

namespace mocainput {


	class DBList {
	public:
		DBList() : m_db_count(0) {};
		~DBList();

		bool add_db(int id, char* path);
		char* get_path(int id) const;
		int m_db_count;


	//private:
		typedef struct {
			int id;
			char* path;
		} db_data;

		db_data m_dbs[MAX_DBS];
	};

	class DBRegistry {
	public:
		char* get_kdb_path(int id) const;
		char* get_ldb_path(int id) const;
        char* get_udb_path(int id) const;
        char* get_asdb_path(int id) const;
        char* get_cdb_path(int id) const;
        char* get_mdb_path(int id) const;
        char* get_hwr_dic_path(int id) const;
        char* get_hwr_db_template_path(int id) const;

        bool isThereKdbPathFor(int id) const;

	//private:
		const static int max_line_len = 256;

		typedef enum {
			STATE_NONE,
			STATE_KDB,
			STATE_LDB,
			STATE_UDB,
			STATE_ASDB,
			STATE_CDB,
			STATE_MDB,
			STATE_HWR_DB_TEMPLATE,
			STATE_HWR_DIC,
		} eState;

		DBList m_ldbs;
		DBList m_kdbs;
		DBList m_udbs;
		DBList m_asdbs;
		DBList m_cdbs;
		DBList m_mdbs;
		DBList m_hwr_dic;
		DBList m_hwr_db_template;

		static bool _is_white_space(char c);

	private:
	      void read_conf_file(const char* fn);

	private:
		DBRegistry(const char* fn);
		~DBRegistry();

	public:
		static int instanceCount;
		static DBRegistry* singleDBRegistryInstance;
		static DBRegistry* getInstance(const char* fn);
		static void deleteInstance();
	};
}
#endif /* __DBREGISTRY_H */
