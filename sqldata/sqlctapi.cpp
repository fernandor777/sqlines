/** 
 * Copyright (c) 2016 SQLines
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// SqlCtApi Sybase CTLIB API

#include "sqlctapi.h"
#include "str.h"
#include "os.h"
#include "file.h"

// Constructor
SqlCtApi::SqlCtApi()
{
	_ct_dll = NULL;
	_cs_dll = NULL;

	_context = NULL;
	_connection = NULL;
	_locale = NULL;

	_cursor_cmd = NULL;
	_cursor_last_fetch_rc = 0;

	_cs_ctx_alloc = NULL;
	_cs_config = NULL;
	_cs_ctx_drop = NULL;
	_cs_dt_info = NULL;
	_cs_loc_alloc = NULL;
	_cs_locale = NULL;
	_ct_bind = NULL;
	_ct_command = NULL;
	_ct_con_alloc = NULL;
	_ct_con_drop = NULL;
	_ct_con_props = NULL;
	_ct_connect = NULL;
	_ct_close = NULL;
	_ct_cmd_alloc = NULL;
	_ct_cmd_drop = NULL;
	_ct_describe = NULL;
	_ct_diag = NULL;
	_ct_exit = NULL;
	_ct_fetch = NULL;
	_ct_init = NULL;
	_ct_res_info = NULL;
	_ct_results = NULL;
	_ct_send = NULL;
}

SqlCtApi::~SqlCtApi()
{
	Deallocate();
}

// Initialize API
int SqlCtApi::Init()
{
#if defined(WIN32) || defined(_WIN64)

	// Try to load the library by default path
	_ct_dll = LoadLibraryEx(CTLIB_DLL, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    _cs_dll = LoadLibraryEx(CSLIB_DLL, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

	// Try a different name from previous versions
	if(_ct_dll == NULL)
    {
		_ct_dll = LoadLibraryEx(CTLIB_DLL2, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
        _cs_dll = LoadLibraryEx(CSLIB_DLL2, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    }                

	// DLL load failed
	if(_ct_dll == NULL)
	{
		std::list<std::string> paths;

		// Try to find Sybase installation paths
		FindSybasePaths(paths);

		for(std::list<std::string>::iterator i = paths.begin(); i != paths.end() ; i++)
		{
			// For Sybase 15 and 16, DDLs are in \ASE-15_0\bin, and cannot be loaded from OCS-15_0 as some dependent DDLs are missed (sybcsi_core27.dll)
			std::string path = (*i) + "\\ASE*";
			std::string dir;

			// Find Sybase Client Library directory
			File::FindDir(path.c_str(), dir);

			if(dir.empty() == false)
			{
				std::string loc_dir = (*i) + '\\';
				loc_dir += dir;
				loc_dir += "\\bin\\";

				std::string loc = loc_dir;
				loc += CTLIB_DLL;
				
				// Try to open DLL
				_ct_dll = LoadLibraryEx(loc.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

				// Try a different name from previous versions
				if(_ct_dll == NULL)
				{
					loc = loc_dir;
					loc += CTLIB_DLL2;

					_ct_dll = LoadLibraryEx(loc.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

					// Load CS library at the same location
					if(_ct_dll != NULL)
					{
						loc = loc_dir;
						loc += CSLIB_DLL2;

						_cs_dll = LoadLibraryEx(loc.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
					}
				}
				else
				// Load CS library at the same location
				{
					loc = loc_dir;
					loc += CSLIB_DLL;

					_cs_dll = LoadLibraryEx(loc.c_str(), NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
				}

				if(_ct_dll != NULL)
					break;
			}
		}		
	}

#endif

	// Get functions
	if(_ct_dll != NULL && _cs_dll != NULL)
	{
		_cs_ctx_alloc = (cs_ctx_allocFunc)Os::GetProcAddress(_cs_dll, "cs_ctx_alloc");
		_cs_config = (cs_configFunc)Os::GetProcAddress(_cs_dll, "cs_config");
		_cs_ctx_drop = (cs_ctx_dropFunc)Os::GetProcAddress(_cs_dll, "cs_ctx_drop");
		_cs_dt_info = (cs_dt_infoFunc)Os::GetProcAddress(_cs_dll, "cs_dt_info");
		_cs_loc_alloc = (cs_loc_allocFunc)Os::GetProcAddress(_cs_dll, "cs_loc_alloc");
		_cs_locale = (cs_localeFunc)Os::GetProcAddress(_cs_dll, "cs_locale");

		_ct_bind = (ct_bindFunc)Os::GetProcAddress(_ct_dll, "ct_bind");
		_ct_command = (ct_commandFunc)Os::GetProcAddress(_ct_dll, "ct_command");
		_ct_con_alloc = (ct_con_allocFunc)Os::GetProcAddress(_ct_dll, "ct_con_alloc");
		_ct_con_drop = (ct_con_dropFunc)Os::GetProcAddress(_ct_dll, "ct_con_drop");
		_ct_con_props = (ct_con_propsFunc)Os::GetProcAddress(_ct_dll, "ct_con_props");
		_ct_connect = (ct_connectFunc)Os::GetProcAddress(_ct_dll, "ct_connect");
		_ct_close = (ct_closeFunc)Os::GetProcAddress(_ct_dll, "ct_close");
		_ct_cmd_alloc = (ct_cmd_allocFunc)Os::GetProcAddress(_ct_dll, "ct_cmd_alloc");
		_ct_cmd_drop = (ct_cmd_dropFunc)Os::GetProcAddress(_ct_dll, "ct_cmd_drop");
		_ct_describe = (ct_describeFunc)Os::GetProcAddress(_ct_dll, "ct_describe");
		_ct_diag = (ct_diagFunc)Os::GetProcAddress(_ct_dll, "ct_diag");
		_ct_exit = (ct_exitFunc)Os::GetProcAddress(_ct_dll, "ct_exit");
		_ct_fetch = (ct_fetchFunc)Os::GetProcAddress(_ct_dll, "ct_fetch");
		_ct_init = (ct_initFunc)Os::GetProcAddress(_ct_dll, "ct_init");
		_ct_res_info = (ct_res_infoFunc)Os::GetProcAddress(_ct_dll, "ct_res_info");
		_ct_results = (ct_resultsFunc)Os::GetProcAddress(_ct_dll, "ct_results");
		_ct_send = (ct_sendFunc)Os::GetProcAddress(_ct_dll, "ct_send");

		if(_cs_ctx_alloc == NULL || _cs_config == NULL || _cs_ctx_drop == NULL || _cs_dt_info == NULL || _cs_locale == NULL || 
			_cs_loc_alloc == NULL || _ct_bind == NULL || _ct_command == NULL || 
			_ct_con_alloc == NULL || _ct_con_drop == NULL || _ct_con_props == NULL || _ct_connect == NULL || 
			_ct_close == NULL || _ct_cmd_alloc == NULL || _ct_cmd_drop == NULL || _ct_describe == NULL || 
			_ct_diag == NULL || _ct_exit == NULL || _ct_fetch == NULL || _ct_init == NULL || 
			_ct_res_info == NULL || _ct_results == NULL || _ct_send == NULL)
			return -1;
	}
	else
	{
		Os::GetLastErrorText(CTLIB_DLL_LOAD_ERROR, _native_error_text, 1024);
		return -1;
	}

	if(_cs_ctx_alloc != NULL)
	{
		CS_RETCODE ret = _cs_ctx_alloc(CS_VERSION_150, &_context);

		if(ret == CS_SUCCEED && _ct_init != NULL)
			ret = _ct_init(_context, CS_VERSION_150);

		_cs_loc_alloc(_context, &_locale);
		_cs_locale(_context, CS_SET, _locale, CS_LC_ALL, (CS_CHAR*)NULL, CS_UNUSED, (CS_INT*)NULL);

		// Define the string format for all data types
		CS_INT dt_fmt_type = CS_DATES_YMDHMSUS_YYYY;
		_cs_dt_info(_context, CS_SET, _locale, CS_DT_CONVFMT, CS_UNUSED, (CS_VOID*)&dt_fmt_type, CS_SIZEOF(CS_INT), NULL);

		// Set the locate with the new format
		_cs_config(_context, CS_SET, CS_LOC_PROP, _locale, CS_UNUSED, NULL);

		return (ret == CS_SUCCEED) ? 0 : -1;
	}

	return 0;
}

// Set the connection string in the API object
void SqlCtApi::SetConnectionString(const char *conn)
{
	if(conn == NULL)
		return;

	std::string db;

	SplitConnectionString(conn, _user, _pwd, db);

	const char *start = db.c_str();

	// Find , that denotes the database name
	const char *comma = strchr(start, ',');

	// Define server and database name
	if(comma != NULL)
	{
		_server.assign(start, (size_t)(comma - start));
		_db = comma + 1;
	}
	else
		_server = start;
}

// Connect to the database
int SqlCtApi::Connect(size_t *time_spent)
{
	if(_connected == true)
		return 0;

	size_t start = (time_spent != NULL) ? Os::GetTickCount() : 0;

	CS_RETCODE rc = _ct_con_alloc(_context, &_connection);

	if(rc != CS_SUCCEED)
		return -1;

	// Initialize inline error handling (without using callbacks)
	rc = _ct_diag(_connection, CS_INIT, CS_UNUSED, CS_UNUSED, NULL);

	// Set user name and password
	rc = _ct_con_props(_connection, CS_SET, CS_USERNAME, (CS_VOID*)_user.c_str(), CS_NULLTERM, NULL);
	rc = _ct_con_props(_connection, CS_SET, CS_PASSWORD, (CS_VOID*)_pwd.c_str(), CS_NULLTERM, NULL);

	const char *server = _server.empty() ? NULL : _server.c_str();

	// Connect to the server
	rc = _ct_connect(_connection, (CS_CHAR*)server, CS_NULLTERM);

	if(rc != CS_SUCCEED)
	{
		SetError();
		return -1;
	}
	
	// Change the current database
	if(_db.empty() == false)
	{
		CS_COMMAND *cmd;
		rc = _ct_cmd_alloc(_connection, &cmd);

		std::string sql = "USE ";
		sql += _db;

		// Add command to the buffer
		rc = _ct_command(cmd, CS_LANG_CMD, (CS_CHAR*)sql.c_str(), CS_NULLTERM, CS_UNUSED);

		// Send command
		if(rc == CS_SUCCEED)
			rc = _ct_send(cmd);

		CS_INT type = 0;

		// Get execution result
		if(rc == CS_SUCCEED)
		{
			while((rc = _ct_results(cmd, &type)) == CS_SUCCEED)
			{
				if(type == CS_CMD_SUCCEED)
					_connected = true;
			}
		}

		rc = _ct_cmd_drop(cmd);
	}
	else
		_connected = true;

	// Set version of the connected database
	SetVersion();

	if(time_spent != NULL)
		*time_spent = Os::GetTickCount() - start;
		
	return (_connected == true) ? 0 : -1;
}

// Disconnect from the database
void SqlCtApi::Disconnect()
{
	if(_connected == false)
		return;

	// Close the connection
	int rc = _ct_close(_connection, CS_UNUSED);

	rc = _ct_con_drop(_connection);
	_connection = NULL;

	_connected = false;
}

// Deallocate the driver
void SqlCtApi::Deallocate()
{
	Disconnect();

	if(_context != NULL)
	{
		int rc = _ct_exit(_context, CS_UNUSED);
		rc = _cs_ctx_drop(_context); 

		_context = NULL;
	}
}

// Get row count for the specified object
int SqlCtApi::GetRowCount(const char *object, int *count, size_t *time_spent)
{
	if(object == NULL)
		return -1;

	std::string query = "SELECT COUNT(*) FROM ";
	query += object;

	// Execute the query
	int rc = ExecuteScalar(query.c_str(), count, time_spent);

	return rc;
}

// Execute the statement and get scalar result
int SqlCtApi::ExecuteScalar(const char *query, int *result, size_t *time_spent)
{
	if(query == NULL || result == NULL)
		return -1;

	size_t start = Os::GetTickCount();

	CS_COMMAND *cmd;

	// Allocate command 
	int rc = _ct_cmd_alloc(_connection, &cmd);

	rc = _ct_command(cmd, CS_LANG_CMD, (CS_CHAR*)query, CS_NULLTERM, CS_UNUSED);

	// Send the command
	if(rc == CS_SUCCEED)
		rc = _ct_send(cmd);

	CS_INT type = 0;

	if(rc != CS_SUCCEED)
	{
		SetError();
		_ct_cmd_drop(cmd);

		return -1;
	}

	int q_result = 0;
	bool exists = false;

	// Process the result set
	while((rc = _ct_results(cmd, &type)) == CS_SUCCEED)
	{
		if(type != CS_ROW_RESULT)
			continue;

		// Fetched data length and NULL indicators
		CS_INT len;
		CS_SMALLINT ind;
	
		CS_DATAFMT fmt = {0};

		fmt.datatype = CS_INT_TYPE;
		fmt.maxlength = sizeof(int);
		fmt.count = 1;
		fmt.locale = NULL;
	
		// Bind the column 
		rc = _ct_bind(cmd, 1, &fmt, &q_result, &len, &ind); 

		CS_INT rows_read = 0;
	
		// Fetch the first row
		if(rc == CS_SUCCEED)
			rc = _ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);

		if(rc == CS_SUCCEED && rows_read > 0)
			exists = true;

		// We must fetch until ct_fetch returns NO DATA
		rc = _ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);
	}

	_ct_cmd_drop(cmd);

	if(exists == true && result != NULL)
		*result = q_result;

	if(time_spent != NULL)
		*time_spent = Os::GetTickCount() - start;

	return (exists == true) ? 0 : -1;
}

// Execute the statement
int SqlCtApi::ExecuteNonQuery(const char *query, size_t *time_spent)
{
	if(query == NULL)
		return -1;

	size_t start = Os::GetTickCount();

	// Execute the 
	//PGresult *result = _PQexec(_conn, query);

	bool error = false;

	// Error raised
	/*if(_PQresultStatus(result) != PGRES_COMMAND_OK)
	{
		SetError();
		error = true;
	}*/

	if(time_spent != NULL)
		*time_spent = Os::GetTickCount() - start;

	//_PQclear(result);

	return (error == true) ? -1 : 0;
}

// Open cursor and allocate buffers
int SqlCtApi::OpenCursor(const char *query, size_t buffer_rows, int buffer_memory, size_t *col_count, size_t *allocated_array_rows, 
		int *rows_fetched, SqlCol **cols, size_t *time_spent, bool catalog_query, std::list<SqlDataTypeMap> * /*dtmap*/)
{
	if(query == NULL) 
		return -1;

	size_t start = Os::GetTickCount();

	// Allocate command 
	int rc = _ct_cmd_alloc(_connection, &_cursor_cmd);

	rc = _ct_command(_cursor_cmd, CS_LANG_CMD, (CS_CHAR*)query, CS_NULLTERM, CS_UNUSED);

	// Send the command
	if(rc == CS_SUCCEED)
		rc = _ct_send(_cursor_cmd);

	CS_INT type = 0;

	if(rc != CS_SUCCEED)
	{
		SetError();
		_ct_cmd_drop(_cursor_cmd);

		return -1;
	}

	// Process the result set (execute ct_result only once)
	if((rc = _ct_results(_cursor_cmd, &type)) != CS_SUCCEED || type != CS_ROW_RESULT)
	{
		SetError();
		_ct_cmd_drop(_cursor_cmd);

		return -1;
	}

	// Define the number of columns
	rc = _ct_res_info(_cursor_cmd, CS_NUMDATA, &_cursor_cols_count, CS_UNUSED, NULL);

	CS_DATAFMT *fmt = NULL;

	if(_cursor_cols_count > 0)
	{
		_cursor_cols = new SqlCol[_cursor_cols_count];
		fmt = new CS_DATAFMT[_cursor_cols_count];
	}

	size_t row_size = 0;

	// Get column information
	for(int i = 0; i < _cursor_cols_count; i++)
	{
		rc = _ct_describe(_cursor_cmd, i + 1, &fmt[i]);

		// Copy the column name
		if(fmt[i].namelen > 0)
		{
			strncpy(_cursor_cols[i]._name, fmt[i].name, (size_t)fmt[i].namelen);
			_cursor_cols[i]._name[fmt[i].namelen] = '\x0';
		}

		// Get column native data type
		_cursor_cols[i]._native_dt = fmt[i].datatype;

		// Get column length for character and binary strings
		_cursor_cols[i]._len = (size_t)fmt[i].maxlength;

		// TEXT, Sybase ASE 16 returns size 32768, change to 1M
		if(_cursor_cols[i]._native_dt == CS_TEXT_TYPE)
			_cursor_cols[i]._len = 1048576;

		row_size += _cursor_cols[i]._len;

		// Get precision and scale for numeric data types 
		_cursor_cols[i]._precision = fmt[i].precision;
		_cursor_cols[i]._scale = fmt[i].scale;

		// Get NOT NULL attribute
		_cursor_cols[i]._nullable = (fmt[i].status & CS_CANBENULL) ? true : false;
	}

	_cursor_allocated_rows = 1;

	// Define how many rows fetch at once
	if(buffer_rows > 0)
		_cursor_allocated_rows = buffer_rows;
	else
	if(buffer_memory > 0)
	{
		size_t rows = buffer_memory/row_size;
		_cursor_allocated_rows = rows > 0 ? rows : 1;
	}	

	// Allocate buffers for each column
	for(int i = 0; i < _cursor_cols_count; i++)
	{
		// CHAR data type
		if(_cursor_cols[i]._native_dt == CS_CHAR_TYPE)
		{
			// Do not bind to null-terminating string as zero byte will be included to length indicator
			_cursor_cols[i]._native_fetch_dt = _cursor_cols[i]._native_dt;
			_cursor_cols[i]._fetch_len = _cursor_cols[i]._len;

			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];
		}
		else
		// BIGINT data type
		if(_cursor_cols[i]._native_dt == CS_BIGINT_TYPE)
		{
			// Bind to string
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;
			_cursor_cols[i]._fetch_len = 21;

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = 21;

			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];
		}
		else
		// INT data type
		if(_cursor_cols[i]._native_dt == CS_INT_TYPE)
		{
			// Check whether target supports bind to INT or string has to be used
			if((_target_api_provider != NULL && _target_api_provider->IsIntegerBulkBindSupported() == true) ||
				catalog_query == true)
			{
				_cursor_cols[i]._native_fetch_dt = _cursor_cols[i]._native_dt;
				_cursor_cols[i]._fetch_len = sizeof(int);
			}
			else
			// Bind to string
			{
				_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;
				_cursor_cols[i]._fetch_len = 11;

				fmt[i].datatype = CS_CHAR_TYPE;
				fmt[i].maxlength = 11;
			}

			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];
		}
		else
		// SMALLINT data type
		if(_cursor_cols[i]._native_dt == CS_SMALLINT_TYPE)
		{
			// Check whether target supports bind to SMALLINT or string has to be used
			if((_target_api_provider != NULL && _target_api_provider->IsSmallintBulkBindSupported() == true) ||
				catalog_query == true)
			{
				_cursor_cols[i]._native_fetch_dt = _cursor_cols[i]._native_dt;
				_cursor_cols[i]._fetch_len = sizeof(short);
			}
			else
			// Bind to string
			{
				_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;
				_cursor_cols[i]._fetch_len = 6;

				fmt[i].datatype = CS_CHAR_TYPE;
				fmt[i].maxlength = 6;
			}

			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];
		}
		else
		// TINYINT, length is 1
		if(_cursor_cols[i]._native_dt == CS_TINYINT_TYPE)
		{
			if(catalog_query)
			{
				_cursor_cols[i]._native_fetch_dt = _cursor_cols[i]._native_dt;
				_cursor_cols[i]._fetch_len = sizeof(char);
			}
			else
			// Bind to string
			{
				_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;
				_cursor_cols[i]._fetch_len = 3;

				fmt[i].datatype = CS_CHAR_TYPE;
				fmt[i].maxlength = 3;
			}

			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];
		}
		else
		// NUMERIC and DECIMAL
		if(_cursor_cols[i]._native_dt == CS_NUMERIC_TYPE || _cursor_cols[i]._native_dt == CS_DECIMAL_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			// Include sign, decimal point and terminating zero
			_cursor_cols[i]._fetch_len = (size_t)(_cursor_cols[i]._precision + _cursor_cols[i]._scale + 3);
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// MONEY, length is 8, precision is 0, scale 0
		if(_cursor_cols[i]._native_dt == CS_MONEY_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._precision = 19;
			_cursor_cols[i]._scale = 4;

			// Include sign, decimal point and terminating zero
			_cursor_cols[i]._fetch_len = 19 + 3;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// SMALLMONEY, length is 4, precision is 0, scale 0
		if(_cursor_cols[i]._native_dt == CS_MONEY4_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._precision = 10;
			_cursor_cols[i]._scale = 4;

			// Include sign, decimal point and terminating zero
			_cursor_cols[i]._fetch_len = 10 + 3;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// DATETIME, length is 8, precision is 0, scale 0
		if(_cursor_cols[i]._native_dt == CS_DATETIME_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._fetch_len = 26;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			_cursor_cols[i]._scale = 3;

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// SMALLDATETIME, length is 4 (up to minute)
		if(_cursor_cols[i]._native_dt == CS_DATETIME4_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._fetch_len = 26;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// DATE, length is 4
		if(_cursor_cols[i]._native_dt == CS_DATE_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			// All date/time data type are fetched in full length, since ANSI ISO format cannot be specified separately for each type
			_cursor_cols[i]._fetch_len = 26;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// FLOAT, length is 8, precision is 0, scale 0
		if(_cursor_cols[i]._native_dt == CS_FLOAT_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._fetch_len = 32;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// BIT, length is 1
		if(_cursor_cols[i]._native_dt == CS_BIT_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._fetch_len = 1;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	
		else
		// TEXT
		if(_cursor_cols[i]._native_dt == CS_TEXT_TYPE)
		{
			_cursor_cols[i]._native_fetch_dt = CS_CHAR_TYPE;

			_cursor_cols[i]._fetch_len = _cursor_cols[i]._len;
			_cursor_cols[i]._data = new char[_cursor_cols[i]._fetch_len * _cursor_allocated_rows];

			fmt[i].datatype = CS_CHAR_TYPE;
			fmt[i].maxlength = (CS_INT)_cursor_cols[i]._fetch_len;
		}	

		// Bind the data to array
		if(_cursor_cols[i]._data != NULL)
		{
			// Allocate indicators
			_cursor_cols[i]._ind2 = new short[_cursor_allocated_rows];
			_cursor_cols[i]._len_ind4 = new int[_cursor_allocated_rows];

			fmt[i].count = (CS_INT)_cursor_allocated_rows;

			rc = _ct_bind(_cursor_cmd, i + 1, &fmt[i], _cursor_cols[i]._data, (CS_INT*)_cursor_cols[i]._len_ind4,
				_cursor_cols[i]._ind2);
		}
	}

	delete [] fmt;

	int fetched = 0;

	// Fetch the initial set of data
	rc = _ct_fetch(_cursor_cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, (CS_INT*)&fetched);

	// CS_END_DATA is returned if there is no rows in the table, 
	// CS_SUCCEED is still returned if there are less rows than allocated buffer 
	if(rc != CS_SUCCEED && rc != CS_END_DATA)
	{
		// Set error message, close cursor and remove buffers
		SetError();
		CloseCursor();

		return -1;
	}

	// Remember last result as we must call ct_fetch until it returns NO DATA
	_cursor_last_fetch_rc = rc;

	if(col_count != NULL)
		*col_count = _cursor_cols_count; 

	if(allocated_array_rows != NULL)
		*allocated_array_rows = _cursor_allocated_rows;

	if(rows_fetched != NULL)
		*rows_fetched = fetched;
	
	if(cols != NULL)
		*cols = _cursor_cols;

	if(time_spent != NULL)
		*time_spent = Os::GetTickCount() - start;

	return (rc == CS_END_DATA) ? 100 : 0;
}

// Fetch next portion of data to allocate buffers
int SqlCtApi::Fetch(int *rows_fetched, size_t *time_spent) 
{
	size_t start = GetTickCount();

	int fetched = 0;

	// Fetch the data
	int rc = _ct_fetch(_cursor_cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, (CS_INT*)&fetched);

	// CS_END_DATA is returned if there is no rows in the table,
	// CS_SUCCEED is still returned if there are less rows than allocated buffer 
	if(rc != CS_SUCCEED && rc != CS_END_DATA)
	{
		SetError();
		return -1;
	}
	
	// Remember last result as we must call ct_fetch until it returns NO DATA
	_cursor_last_fetch_rc = rc;

	if(rows_fetched != NULL)
		*rows_fetched = fetched;

	if(time_spent != NULL)
		*time_spent = GetTickCount() - start;
	
	return (rc == CS_END_DATA) ? 100 : 0;
}

// Close the cursor and deallocate buffers
int SqlCtApi::CloseCursor()
{
	CS_INT type;
	int rc = 0;

	int fetched = 0;

	// We must call ct_fetch until it returns NO DATA
	if(_cursor_last_fetch_rc == CS_SUCCEED)
		rc = _ct_fetch(_cursor_cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, (CS_INT*)&fetched);

	bool more = true;

	// Consume all results
	while(more)
	{
		rc = _ct_results(_cursor_cmd, &type);

		if(rc == CS_FAIL)
			SetError();
		
		if(rc != CS_SUCCEED)
		{
			more = false;
			break;
		}
	}
	
	rc = _ct_cmd_drop(_cursor_cmd);
	
	_cursor_cmd = NULL;

	// Delete allocated buffers
	for(int i = 0; i < _cursor_cols_count; i++)
	{
		delete [] _cursor_cols[i]._data;
		delete [] _cursor_cols[i]._ind2;
		delete [] _cursor_cols[i]._len_ind4;
	}

	delete [] _cursor_cols;

	_cursor_cols = NULL;
	_cursor_cols_count = 0;
	_cursor_allocated_rows = 0;

	return 0;
}

// Initialize the bulk copy from one database into another
int SqlCtApi::InitBulkTransfer(const char * /*table*/, size_t /*col_count*/, size_t /*allocated_array_rows*/, SqlCol * /*s_cols*/, SqlCol ** /*t_cols*/)
{
	return -1;
}

// Transfer rows between databases
int SqlCtApi::TransferRows(SqlCol * /*s_cols*/, int /*rows_fetched*/, int * /*rows_written*/, size_t * /*bytes_written*/,
							size_t * /*time_spent*/)
{
	return -1;
}

// Write LOB data using BCP API
int SqlCtApi::WriteLob(SqlCol * /*s_cols*/, int /*row*/, int * /*lob_bytes*/)
{
	return -1;
}

// Complete bulk transfer
int SqlCtApi::CloseBulkTransfer()
{
	return -1;
}

// Drop the table
int SqlCtApi::DropTable(const char* table, size_t *time_spent, std::string &drop_stmt)
{
	drop_stmt = "DROP TABLE ";
	drop_stmt += table;

	int rc = ExecuteNonQuery(drop_stmt.c_str(), time_spent);

	return rc;
}	

// Remove foreign key constraints referencing to the parent table
int SqlCtApi::DropReferences(const char* /*table*/, size_t * /*time_spent*/)
{
	return -1;
}

// Get the length of LOB column in the open cursor
int SqlCtApi::GetLobLength(size_t /*row*/, size_t /*column*/, size_t * /*length*/)
{
	return -1;
}

// Get LOB content
int SqlCtApi::GetLobContent(size_t /*row*/, size_t /*column*/, void * /*data*/, size_t /*length*/, int * /*len_ind*/)
{
	return -1;
}

// Get the list of available tables
int SqlCtApi::GetAvailableTables(std::string &table_template, std::string & /*exclude*/, 
									std::list<std::string> &tables)
{
	std::string condition;

	// Get a condition to select objects from the catalog
	GetSelectionCriteria(table_template.c_str(), "USER_NAME(uid)", "name", condition, NULL, false);
	
	CS_COMMAND *cmd;

	// Allocate command 
	int rc = _ct_cmd_alloc(_connection, &cmd);

	// Build the query
	std::string sql = "SELECT USER_NAME(uid), name FROM sysobjects WHERE type = 'U'";

	// Add filter
	if(condition.empty() == false)
	{
		sql += " AND ";
		sql += condition;
	}

	rc = _ct_command(cmd, CS_LANG_CMD, (CS_CHAR*)sql.c_str(), CS_NULLTERM, CS_UNUSED);

	// Send the command
	if(rc == CS_SUCCEED)
		rc = _ct_send(cmd);

	CS_INT type = 0;

	if(rc != CS_SUCCEED)
	{
		SetError();
		_ct_cmd_drop(cmd);

		return -1;
	}

	// Process the result set
	while((rc = _ct_results(cmd, &type)) == CS_SUCCEED)
	{
		if(type != CS_ROW_RESULT)
			continue;

		// Fetched data
		char owner[100][255];
		char table_name[100][255];

		// Fetched data length and NULL indicators
		CS_INT owner_len[100], table_name_len[100];
		CS_SMALLINT owner_ind[100], table_name_ind[100];
	
		CS_DATAFMT fmt;

		fmt.datatype = CS_CHAR_TYPE;
		fmt.maxlength = 255;
		fmt.format = CS_FMT_NULLTERM;
		fmt.count = 100;
		fmt.locale = NULL;
	
		// Bind owner and table name
		rc = _ct_bind(cmd, 1, &fmt, owner, owner_len, owner_ind); 
		rc = _ct_bind(cmd, 2, &fmt, table_name, table_name_len, table_name_ind); 

		CS_INT rows_read = 0;

		bool more = true;
	
		// Fetch rows
		while(more)
		{
			rc = _ct_fetch(cmd, CS_UNUSED, CS_UNUSED, CS_UNUSED, &rows_read);

			if(rc != CS_SUCCEED) 
			{
				more = false;
				break;
			}

			// Output fetched data 
			for(int i = 0; i < rows_read; i++)
			{
				std::string tab = owner[i];
				tab += ".";
				tab += table_name[i];

				tables.push_back(tab);
			}
		}
	}

	_ct_cmd_drop(cmd);

	return 0;
}

// Read schema information
int SqlCtApi::ReadSchema(const char *select, const char *exclude, bool read_cns, bool read_idx)
{
	std::string condition;

	ClearSchema();

	// Get a condition to select objects from the catalog
	// User can see multiple schemas in the database
	// * means all user tables, *.* means all tables accessible by the user
	GetSelectionCriteria(select, exclude, "u.name", "o.name", condition, "dbo", false);

	int rc = ReadTableColumns(condition);

	// Primary and unique key constraints are read from indexes
	if(read_cns || read_idx)
		rc = ReadIndexes(condition);

	if(read_cns)
		rc = ReadReferences(condition);

	return rc;
}

// Read information about table columns
int SqlCtApi::ReadTableColumns(std::string &condition)
{
	// Tested on Sybase ASE 16
	// Note that systypes contains duplicate type, only usertype is unique
	// Do **not** join on 2 columns: c.type = t.type AND c.usertype = t.usertype - it does not match (tested on pubs3 and some columns can be lost)
	std::string query = "select u.name, o.name, c.name, t.name, c.colid, o.id, c.status"; 
    query += " FROM sysusers u, sysobjects o, syscolumns c, systypes t";
	query += " WHERE o.type = 'U' AND o.uid = u.uid AND o.id = c.id AND c.usertype = t.usertype";

	if(condition.empty() == false)
	{
		query += " AND ";
		query += condition;
	}
	
	query += " ORDER BY o.id, c.colid";
	
	size_t col_count = 0;
	size_t allocated_rows = 0;
	int rows_fetched = 0; 
	size_t time_read = 0;
	
	SqlCol *cols = NULL;
	SqlColMeta col_meta;

	// Open cursor allocating 100 rows buffer
	int rc = OpenCursor(query.c_str(), 100, 0, &col_count, &allocated_rows, &rows_fetched, &cols, 
		&time_read, true);

	while(rc >= 0)
	{
		// Copy column information
		for(int i = 0; i < rows_fetched; i++)
		{
			SQLLEN len;

			col_meta.schema = NULL;
			col_meta.table = NULL;
			col_meta.column = NULL;
			col_meta.num = 0;
			col_meta.tabid = 0;
			col_meta.data_type = NULL;
			col_meta.identity = false;
			col_meta.id_start = 0;
			col_meta.id_inc = 0;
			
			len = GetLen(&cols[0], i);
			
			// Schema
			if(len != -1)
			{
				col_meta.schema = new char[(size_t)len + 1];

				strncpy(col_meta.schema, cols[0]._data + cols[0]._fetch_len * i, (size_t)len);
				col_meta.schema[len] = '\x0';
			}

			len = GetLen(&cols[1], i);

			// Table
			if(len != -1)
			{
				col_meta.table = new char[(size_t)len + 1];

				strncpy(col_meta.table, cols[1]._data + cols[1]._fetch_len * i, (size_t)len);
				col_meta.table[len] = '\x0';
			}

			len = GetLen(&cols[2], i);

			// Column
			if(len != -1)
			{
				col_meta.column = new char[(size_t)len + 1];

				strncpy(col_meta.column, cols[2]._data + cols[2]._fetch_len * i, (size_t)len);
				col_meta.column[len] = '\x0';
			}

			len = GetLen(&cols[3], i);

			// Data type
			if(len != -1)
			{
				col_meta.data_type = new char[(size_t)len + 1];

				strncpy(col_meta.data_type, cols[3]._data + cols[3]._fetch_len * i, (size_t)len);
				col_meta.data_type[len] = '\x0';
			}
						
			len = GetLen(&cols[4], i);

			// Column number (SMALLINT)
			if(len == 2)
				col_meta.num = *((short*)(cols[4]._data + cols[4]._fetch_len * i));

			len = GetLen(&cols[5], i);

			// Table ID (INTEGER)
			if(len == 4)
				col_meta.tabid = *((int*)(cols[5]._data + cols[5]._fetch_len * i));

			len = GetLen(&cols[6], i);

			// Status (TINYINT)
			if(len == 1)
			{
				char status = *((char*)(cols[6]._data + cols[6]._fetch_len * i));

				// Identity column
				if(status & 0x80)
				{
					col_meta.identity = true;
					col_meta.id_start = 1;
					col_meta.id_inc = 1;
				}
			}
			
			_table_columns.push_back(col_meta);
		}

		rc = Fetch(&rows_fetched, &time_read);

		// No more rows
		if(rc == 100)
			break;
	}

	// Set pointer to NULL to avoid delete if destructor (values belong to list now)
	col_meta.schema = NULL; 
	col_meta.table = NULL; 
	col_meta.column = NULL; 
	col_meta.data_type = NULL; 
	col_meta.default_value = NULL;

	CloseCursor();

	return 0;
}

// Read information about indexes including primary and unique key constraints
int SqlCtApi::ReadIndexes(std::string &condition)
{
	// Tested on Sybase ASE 16
	std::string query = "select u.name, o.name, i.name, i.status, i.id, i.indid, index_col(object_name(i.id), indid, 1), index_col(object_name(i.id), indid, 2),";
	query += " index_col(object_name(i.id), indid, 3), index_col(object_name(i.id), indid, 4), index_col(object_name(i.id), indid, 5),";
	query += " index_col(object_name(i.id), indid, 6), index_col(object_name(i.id), indid, 7), index_col(object_name(i.id), indid, 8)";
	query += " from sysindexes i, sysobjects o, sysusers u";
	query += " where i.indid <> 0 and i.id = o.id and o.type = 'U' and indid <> 255 and substring(object_name(i.id),1,3) <> 'sys' and o.uid = u.uid ";

	if(condition.empty() == false)
	{
		query += " AND ";
		query += condition;
	}
	
	size_t col_count = 0;
	size_t allocated_rows = 0;
	int rows_fetched = 0; 
	size_t time_read = 0;
	
	SqlCol *cols = NULL;
	SqlConstraints col_cns;

	// Open cursor allocating 100 rows buffer
	int rc = OpenCursor(query.c_str(), 100, 0, &col_count, &allocated_rows, &rows_fetched, &cols, 
		&time_read, true);

	while(rc >= 0)
	{
		// Copy column information
		for(int i = 0; i < rows_fetched; i++)
		{
			SQLLEN len;

			col_cns.schema = NULL; 
			col_cns.table = NULL; 
			col_cns.constraint = NULL;
			col_cns.type = '\x0';
			col_cns.tabid = 0;

			len = GetLen(&cols[0], i);
			
			// Schema
			if(len != -1)
			{
				col_cns.schema = new char[(size_t)len + 1];

				strncpy(col_cns.schema, cols[0]._data + cols[0]._fetch_len * i, (size_t)len);
				col_cns.schema[len] = '\x0';
			}

			len = GetLen(&cols[1], i);

			// Table
			if(len != -1)
			{
				col_cns.table = new char[(size_t)len + 1];

				strncpy(col_cns.table, cols[1]._data + cols[1]._fetch_len * i, (size_t)len);
				col_cns.table[len] = '\x0';
			}

			len = GetLen(&cols[2], i);
			
			// Index (constraint) name
			if(len != -1)
			{
				col_cns.constraint = new char[(size_t)len + 1];

				strncpy(col_cns.constraint, cols[2]._data + cols[2]._fetch_len * i, (size_t)len);
				col_cns.constraint[len] = '\x0';
			}

			len = GetLen(&cols[3], i);

			// Status (SMALLINT) - bits: 0x2 - Unique index, 0x800 - Index on primary key
			if(len == 2)
			{
				short status = *((short*)(cols[3]._data + cols[3]._fetch_len * i));

				// Primary key
				if(status & 0x800)
					col_cns.type = 'P';
				else
				// Unique index
				if(status & 0x2)
					col_cns.type = 'U';
			}

			len = GetLen(&cols[4], i);

			// Table ID (INTEGER)
			if(len == 4)
				col_cns.tabid = *((int*)(cols[4]._data + cols[4]._fetch_len * i));

			len = GetLen(&cols[5], i);

			// Index ID (SMALLINT)
			if(len == 2)
				col_cns.idxid = *((short*)(cols[5]._data + cols[5]._fetch_len * i));

			if(col_cns.type != '\x0')
				_table_constraints.push_back(col_cns);

			// Define index columns
			for(int k = 0; k < 8; k++)
			{
				len = GetLen(&cols[6 + k], i);

				// No more columns in the index
				if(len == -1)
					break;

				SqlIndColumns idx_col;

				idx_col.index = (char*)Str::GetCopy(col_cns.constraint);
				idx_col.tabid = col_cns.tabid;
				idx_col.idxid = col_cns.idxid;

				// Column name
				idx_col.column = new char[(size_t)len + 1];

				strncpy(idx_col.column, cols[6 + k]._data + cols[6 + k]._fetch_len * i, (size_t)len);
				idx_col.column[len] = '\x0';

				_table_ind_columns.push_back(idx_col);

				// Pointers now belong to the list
				idx_col.index = NULL;
				idx_col.column = NULL;
			}
		}

		rc = Fetch(&rows_fetched, &time_read);

		// No more rows
		if(rc == 100)
			break;
	}

	// Set pointer to NULL to avoid delete if destructor (values belong to list now)
	col_cns.schema = NULL; 
	col_cns.table = NULL; 
	col_cns.constraint = NULL; 
	col_cns.condition = NULL; 
	col_cns.r_schema = NULL; 
	col_cns.r_constraint = NULL;
	col_cns.idxname = NULL;

	CloseCursor();

	return 0;
}

// Read information about foreign keys
int SqlCtApi::ReadReferences(std::string &condition)
{
	// Tested on Sybase ASE 16
	std::string query = "select pu.name, po.name, u.name, o.name, co.name, r.tableid, r.constrid,";
	query += " col_name(r.tableid,fokey1), col_name(r.tableid,fokey2), col_name(r.tableid,fokey3), col_name(r.tableid,fokey4), col_name(r.tableid,fokey5),";
	query += " col_name(r.tableid,fokey6), col_name(r.tableid,fokey7), col_name(r.tableid,fokey8), col_name(r.tableid,fokey9), col_name(r.tableid,fokey10),";
	query += " col_name(r.tableid,fokey11), col_name(r.tableid,fokey12), col_name(r.tableid,fokey13), col_name(r.tableid,fokey14), col_name(r.tableid,fokey15),";
	query += " col_name(r.tableid,fokey16),"; 
	query += " col_name(r.reftabid,refkey1), col_name(r.reftabid,refkey2), col_name(r.reftabid,refkey3), col_name(r.reftabid,refkey4), col_name(r.reftabid,refkey5),";
	query += " col_name(r.reftabid,refkey6), col_name(r.reftabid,refkey7), col_name(r.reftabid,refkey8), col_name(r.reftabid,refkey9), col_name(r.reftabid,refkey10),";
	query += " col_name(r.reftabid,refkey11), col_name(r.reftabid,refkey12), col_name(r.reftabid,refkey13), col_name(r.reftabid,refkey14), col_name(r.reftabid,refkey15),";
	query += " col_name(r.reftabid,refkey16)"; 
	query += " from sysreferences r, sysobjects po, sysusers pu, sysobjects o, sysusers u, sysobjects co";
	query += " where r.reftabid = po.id and po.uid = pu.uid and r.tableid = o.id and o.uid = u.uid and r.constrid = co.id";

	if(condition.empty() == false)
	{
		query += " AND ";
		query += condition;
	}
	
	size_t col_count = 0;
	size_t allocated_rows = 0;
	int rows_fetched = 0; 
	size_t time_read = 0;
	
	SqlCol *cols = NULL;
	SqlConstraints cns;

	// Open cursor allocating 100 rows buffer
	int rc = OpenCursor(query.c_str(), 100, 0, &col_count, &allocated_rows, &rows_fetched, &cols, 
		&time_read, true);

	while(rc >= 0)
	{
		// Copy column information
		for(int i = 0; i < rows_fetched; i++)
		{
			SQLLEN len;

			cns.schema = NULL; 
			cns.table = NULL; 
			cns.constraint = NULL;
			cns.type = 'R';
			cns.tabid = 0;
			cns.pk_schema = NULL; 
			cns.pk_table = NULL;

			len = GetLen(&cols[0], i);
			
			// Primary key table schema
			if(len != -1)
			{
				cns.pk_schema = new char[(size_t)len + 1];

				strncpy(cns.pk_schema, cols[0]._data + cols[0]._fetch_len * i, (size_t)len);
				cns.pk_schema[len] = '\x0';
			}

			len = GetLen(&cols[1], i);

			// Primary key table name
			if(len != -1)
			{
				cns.pk_table = new char[(size_t)len + 1];

				strncpy(cns.pk_table, cols[1]._data + cols[1]._fetch_len * i, (size_t)len);
				cns.pk_table[len] = '\x0';
			}

			len = GetLen(&cols[2], i);
			
			// Schema
			if(len != -1)
			{
				cns.schema = new char[(size_t)len + 1];

				strncpy(cns.schema, cols[2]._data + cols[2]._fetch_len * i, (size_t)len);
				cns.schema[len] = '\x0';
			}

			len = GetLen(&cols[3], i);

			// Table
			if(len != -1)
			{
				cns.table = new char[(size_t)len + 1];

				strncpy(cns.table, cols[3]._data + cols[3]._fetch_len * i, (size_t)len);
				cns.table[len] = '\x0';
			}

			len = GetLen(&cols[4], i);
			
			// Constraint name
			if(len != -1)
			{
				cns.constraint = new char[(size_t)len + 1];

				strncpy(cns.constraint, cols[4]._data + cols[4]._fetch_len * i, (size_t)len);
				cns.constraint[len] = '\x0';
			}

			len = GetLen(&cols[5], i);

			// Table ID (INTEGER)
			if(len == 4)
				cns.tabid = *((int*)(cols[5]._data + cols[5]._fetch_len * i));

			len = GetLen(&cols[6], i);

			// Constraint ID (INTEGER)
			if(len == 4)
				cns.cnsid = *((int*)(cols[6]._data + cols[6]._fetch_len * i));

			_table_constraints.push_back(cns);

			// Define columns
			for(int k = 0; k < 16; k++)
			{
				len = GetLen(&cols[7 + k], i);

				// No more columns in the foreign key - COL_NAME() returns NULL when applied for column ID 0
				if(len == -1)
					break;

				SqlConsColumns cns_col;

				cns_col.schema = (char*)Str::GetCopy(cns.schema);
				cns_col.table = (char*)Str::GetCopy(cns.table);
				cns_col.constraint = (char*)Str::GetCopy(cns.constraint);

				cns_col.tabid = cns.tabid;
				cns_col.cnsid = cns.cnsid;

				// FK column name
				cns_col.column = new char[(size_t)len + 1];

				strncpy(cns_col.column, cols[7 + k]._data + cols[7 + k]._fetch_len * i, (size_t)len);
				cns_col.column[len] = '\x0';

				len = GetLen(&cols[7 + 16 + k], i);

				// PK column name
				if(len != -1)
				{
					cns_col.pk_column = new char[(size_t)len + 1];

					strncpy(cns_col.pk_column, cols[7 + 16 + k]._data + cols[7 + 16 + k]._fetch_len * i, (size_t)len);
					cns_col.pk_column[len] = '\x0';
				}

				_table_cons_columns.push_back(cns_col);

				// Pointers now belong to the list
				cns_col.schema = NULL;
				cns_col.table = NULL;
				cns_col.constraint = NULL;
				cns_col.column = NULL;
				cns_col.pk_column = NULL;
			}
		}

		rc = Fetch(&rows_fetched, &time_read);

		// No more rows
		if(rc == 100)
			break;
	}

	// Set pointer to NULL to avoid delete if destructor (values belong to list now)
	cns.schema = NULL; 
	cns.table = NULL; 
	cns.constraint = NULL; 
	cns.condition = NULL; 
	cns.r_schema = NULL; 
	cns.r_constraint = NULL;
	cns.idxname = NULL;
	cns.pk_schema = NULL; 
	cns.pk_table = NULL;
	CloseCursor();

	return 0;
}

// Get table name by constraint name
int SqlCtApi::ReadConstraintTable(const char * /*schema*/, const char * /*constraint*/, std::string & /*table*/)
{
	return -1;
}

// Read information about constraint columns
int SqlCtApi::ReadConstraintColumns(const char * /*schema*/, const char * /*table*/, const char * /*constraint*/, std::string & /*cols*/)
{
	return -1;
}

// Get a list of columns for specified primary or unique key
int SqlCtApi::GetKeyConstraintColumns(SqlConstraints &cns, std::list<std::string> &output, std::list<std::string> *)
{
	bool found = false;

	// Find index columns
	for(std::list<SqlIndColumns>::iterator i = _table_ind_columns.begin(); i != _table_ind_columns.end(); i++)
	{
		if((*i).tabid != cns.tabid)
		{
			// Not found yet
			if(found == false)
				continue;
			// Next index
			else
				break;
		}

		// idxid is meaningful within the same tabid only
		if((*i).idxid == cns.idxid)
		{
			output.push_back((*i).column);
			found = true;
		}
	}
				
	return 0;
}

// Get a list of columns for specified foreign key
int SqlCtApi::GetForeignKeyConstraintColumns(SqlConstraints &cns, std::list<std::string> &fcols, std::list<std::string> &pcols, std::string &ptable)
{
	ptable = cns.pk_schema;
	ptable += ".";
	ptable += cns.pk_table;

	bool found = false;

	// Find columns
	for(std::list<SqlConsColumns>::iterator i = _table_cons_columns.begin(); i != _table_cons_columns.end(); i++)
	{
		if((*i).cnsid == cns.cnsid && (*i).tabid == cns.tabid)
		{
			fcols.push_back((*i).column);
			pcols.push_back((*i).pk_column);
		}
		else if(found)
			break;
	}

	return 0;
}

// Build a condition to select objects from the catalog
void SqlCtApi::GetCatalogSelectionCriteria(std::string & /*selection_template*/, std::string & /*output*/)
{
}

// Set version of the connected database
void SqlCtApi::SetVersion()
{
	// Tested on Sybase 16
	std::string query = "SELECT @@version";

	size_t col_count = 0;
	size_t allocated_rows = 0;
	int rows_fetched = 0; 
	size_t time_read = 0;
	
	SqlCol *cols = NULL;

	// Open cursor 
	int rc = OpenCursor(query.c_str(), 1, 0, &col_count, &allocated_rows, &rows_fetched, &cols, &time_read, true);

	// Get the version value
	if(rc >= 0 && cols != NULL && cols[0]._ind2 != NULL && cols[0]._ind2[0] != -1)
		_version.append(cols[0]._data, (size_t)cols[0]._len_ind4[0]);

	CloseCursor();
}

// Get length or NULL
SQLLEN SqlCtApi::GetLen(SqlCol *col, int offset)
{
	if(col != NULL && col->_ind2 != NULL && col->_ind2[offset] != -1)
		return col->_len_ind4[offset];

	return -1;
}

// Find Sybase installation paths
void SqlCtApi::FindSybasePaths(std::list<std::string> &paths)
{
#if defined(WIN32) || defined(_WIN64)
	HKEY hKey;

	char location[1024];
	int value_size = 1024;

	// At least for Sybase 15 server installation
	int rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Sybase\\Setup", 0, KEY_READ, &hKey);

	if(rc == ERROR_SUCCESS)
	{
		// For 15.0 "Sybase" parameter contains installation directory without terminating '\'
	    rc = RegQueryValueEx(hKey, "Sybase", NULL, NULL, (LPBYTE)location, (LPDWORD)&value_size); 

	    if(rc == ERROR_SUCCESS)
			paths.push_back(std::string(location));

		RegCloseKey(hKey);
		return;
	}

	// Sybase 16 server installation
	rc = RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\Sybase\\SQLServer", 0, KEY_READ, &hKey);

	if(rc == ERROR_SUCCESS)
	{
		// For 16 "RootDir" parameter contains installation directory without terminating '\'
	    rc = RegQueryValueEx(hKey, "RootDir", NULL, NULL, (LPBYTE)location, (LPDWORD)&value_size); 

	    if(rc == ERROR_SUCCESS)
			paths.push_back(std::string(location));

		RegCloseKey(hKey);
		return;
	}
#endif
}

// Set error code and message for the last API call
void SqlCtApi::SetError()
{
	CS_CLIENTMSG msg;
	CS_CLIENTMSG smsg;

	// Try to get client message (returned by ct_connect i.e.)
	CS_RETCODE rc = _ct_diag(_connection, CS_GET, CS_CLIENTMSG_TYPE, 1, &msg);
	
	// Try to get server message (returned by ct_results i.e.)
	if(rc != CS_SUCCEED)
	{
		rc = _ct_diag(_connection, CS_GET, CS_SERVERMSG_TYPE, 1, &smsg);

		if(rc == CS_SUCCEED)
			strcpy(_native_error_text, smsg.msgstring);
	}
	else
		strcpy(_native_error_text, msg.msgstring);
	
	_error = SQL_DBAPI_UNKNOWN_ERROR;
	*_error_text = '\x0';
}
