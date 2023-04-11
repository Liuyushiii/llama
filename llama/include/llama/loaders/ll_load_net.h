/*
 * ll_load_net.h
 * LLAMA Graph Analytics
 *
 * Copyright 2014
 *      The President and Fellows of Harvard College.
 *
 * Copyright 2014
 *      Oracle Labs.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE UNIVERSITY AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE UNIVERSITY OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */


#ifndef LL_LOAD_NET_H_
#define LL_LOAD_NET_H_

#include <sys/types.h>
#include <sys/stat.h>
#include <cstdio>
#include <sstream>
#include <unistd.h>
#include <iostream>

#include "llama/ll_mem_array.h"
#include "llama/ll_writable_graph.h"

#include "llama/loaders/ll_load_async_writable.h"
#include "llama/loaders/ll_load_utils.h"


/**
 * The SNAP .net loader
 */
class ll_loader_net : public ll_file_loader {

public:

	/**
	 * Create a new instance of ll_loader_net
	 */
	ll_loader_net() {}


	/**
	 * Destroy the instance
	 */
	virtual ~ll_loader_net() {}


	/**
	 * Determine if this file can be opened by this loader
	 *
	 * @param file the file
	 * @return true if it can be opened
	 */
	virtual bool accepts(const char* file) {

		return strcmp(ll_file_extension(file), "net") == 0
			|| strcmp(ll_file_extension(file), "snap") == 0;
	}


	/**
	 * Load directly into the read-only representation by creating a new
	 * level
	 *
	 * @param graph the graph
	 * @param file the file
	 * @param config the loader configuration
	 */
	virtual void load_direct(ll_mlcsr_ro_graph* graph, const char* file,
			const ll_loader_config* config) {

		ll_loader_config c;
		if (config != NULL) c = *config;
		c.lc_partial_load_part = 0;
		c.lc_partial_load_num_parts = 0;

		net_loader loader(file, config);

		bool r = loader.load_direct(graph, &c);
		if (!r) abort();
	}


	/**
	 * Load incrementally into the writable representation
	 *
	 * @param graph the graph
	 * @param file the file
	 * @param config the loader configuration
	 */
	virtual void load_incremental(ll_writable_graph* graph, const char* file,
			const ll_loader_config* config) {

		ll_loader_config c;
		if (config != NULL) c = *config;
		c.lc_partial_load_part = 0;
		c.lc_partial_load_num_parts = 0;

		net_loader loader(file, config);

		bool r = loader.load_incremental(graph, &c);
		if (!r) abort();
	}


	/**
	 * Create a data source object for the given file
	 *
	 * @param file the file
	 * @return the data source
	 */
	virtual ll_data_source* create_data_source(const char* file) {
		return new net_loader(file);
	}


private:

	/**
	 * The direct loader for X-Stream Type 1 files
	 */
	class net_loader : public ll_edge_list_loader<unsigned, false>
	{	

		std::string _file_name;
		FILE* _file;

		size_t _line_n;
		char* _line;

		ll_loader_config _config;

		off_t _start_offset;
		off_t _stop_offset;	/* including the line that _stop_offset-1 points to */
		off_t _offset;
		bool _done;

		size_t _file_size;
		size_t _errors;

		size_t _max_allowed_errors;


	public:

		/**
		 * Create an instance of class net_loader
		 *
		 * @param file_name the file name
		 * @param config the loader config
		 */
		net_loader(const char* file_name, const ll_loader_config* config = NULL)
			: ll_edge_list_loader<unsigned, false>() {

			if (config != NULL) {
				_config = *config;

				if (_config.lc_partial_load_num_parts > 0) {
					if (_config.lc_partial_load_part <= 0
							|| (_config.lc_partial_load_part
								> _config.lc_partial_load_num_parts)) {
						LL_E_PRINT("The partial load part ID is out of bounds\n");
						abort();
					}
				}
			}

			_file_name = file_name;
			int f = open(file_name, O_RDONLY);
			if (f < 0) {
				perror("Cannot open the input file");
				abort();
			}

			struct stat st;
			if (fstat(f, &st) != 0) {
				perror("Cannot stat the input file");
				abort();
			}

			_file_size = st.st_size;

			_file = fdopen(f, "rt");
			if (_file == NULL) {
				perror("Cannot open the input stream");
				abort();
			}

			if (_config.lc_partial_load_num_parts > 0) {
				_start_offset = st.st_size * (_config.lc_partial_load_part - 1)
					/ _config.lc_partial_load_num_parts;
				_stop_offset = st.st_size * _config.lc_partial_load_part
					/ _config.lc_partial_load_num_parts;
				rewind();
			}
			else {
				_start_offset = 0;
				_stop_offset = 0; //st.st_size;
			}

			_line_n = 64;
			_line = (char*) malloc(_line_n);

			_offset = 0;
			_errors = 0;
			_done = false;
			_max_allowed_errors = 100;		// TODO Should be configurable
		}


		/**
		 * Destroy the loader
		 */
		virtual ~net_loader() {
			if (_file != NULL) fclose(_file);
			if (_line != NULL) free(_line);
		}


	protected:

		/**
		 * Read the next edge
		 *
		 * @param o_tail the output for tail
		 * @param o_head the output for head
		 * @param o_weight the output for weight (ignore if HasWeight is false)
		 * @return true if the edge was loaded, false if EOF or error
		 */
		virtual bool next_edge(unsigned* o_tail, unsigned* o_head,
				float* o_weight) {

			if (_done) return false;

			ssize_t r = net_next_line(_file, &_line, &_line_n, o_tail, o_head);
			if (r < 0) {
				_offset = ftell(_file);
				_done = true;
				return false;
			}

			_offset += r;

			if (_stop_offset > 0) {
				if (_offset >= _stop_offset) _done = true;
			}

			return true;
		}


		/**
		 * Rewind the input file
		 */
		virtual void rewind() {

			_done = false;
			_offset = _start_offset;
			_errors = 0;

			if (_start_offset == 0) {
				std::rewind(_file);
			}
			else {

				fseek(_file, _start_offset - 1, SEEK_SET);
				_offset--;

				int c;
				while ((c = fgetc(_file)) != EOF) {
					_offset++;
					if (c == '\n') break;
				}
			}
		}


		/**
		 * Get next line from the .net file
		 *
		 * @param fin the file
		 * @param p_line the pointer to a malloc-ed line buffer
		 * @param p_line_len the pointer to the line buffer size
		 * @param p_tail the tail out pointer
		 * @param p_head the head out pointer
		 * @return the number of bytes read, or a negative number on error or EOF
		 */
		ssize_t net_next_line(FILE* fin, char** p_line, size_t* p_line_len,
				unsigned* p_tail, unsigned* p_head) {

			// std::cout << "net_next_line" << std::endl;

			ssize_t read;
			ssize_t read_total = 0;

			// 函数会先在 p_line 中读取一行数据，p_line 是一个指向字符指针的指针，指向的字符指针指向保存读取行数据的字符数组。
			// p_line_len 是一个指向 size_t 类型的指针，指向保存行数据字符数组长度的变量。
			while ((read = getline(p_line, p_line_len, fin)) != -1) {

				// 在读取到数据后，函数会将读取到的字符数组赋值给 line 变量。
				
				// std::cout << *p_line << std::endl;


				char* line = *p_line;
				read_total += read;

				// 函数会检查这行数据的开头是否为空格、#、换行符或回车符，如果是，则跳过该行数据继续读取下一行。
				if (*line == '\0' || *line == '#'
						|| *line == '\n' || *line == '\r') continue;


				// 如果这行数据开头不是空格、#、换行符或回车符，函数会从这行数据中提取出两个整数值：p_tail 和 p_head，并将它们赋值给相应的指针。
				char* s = line;
				char* e;

				while (isspace(*s)) s++;

				// 函数会通过 strtol 函数将字符数组中的字符串转换成整数值，第二个参数 e 是一个指向字符指针的指针，
				// strtol 函数会将解析后的字符串的末尾位置赋值给 e，如果解析失败，则会返回 parse_error 函数并跳过该行数据。
				

				// long int strtol(const char* str, char** endptr, int base);
				// 	str：要转换的字符串
				// 	endptr：指向一个指针，指针指向的位置用于保存第一个未被转换的字符的指针（即解析停止的位置），可以为 NULL
				// 	base：转换的基数，如 2、8、10 或 16，其中 0 表示自动判断（十进制或十六进制）

				*p_tail = strtol(s, &e, 10);
				if (s == e || *e == '\0' || !isspace(*e)) {
					parse_error(line);
					continue;
				}

				s = e + 1;
				while (isspace(*s)) s++;

				*p_head = strtol(s, &e, 10);
				if (s == e || (*e != '\0' && *e != '\r' && *e != '\n')) {
					parse_error(line);
					continue;
				}

				// 如果解析成功，则函数会返回读取的字符数。如果没有更多的行可以读取，则函数会返回 -1。
				return read_total;
			}

			return -1l;
		}


	private:

		/**
		 * Handle a parse error
		 *
		 * @param line the line buffer (will be modified!)
		 */
		void parse_error(char* line) {

			char* p;
			if ((p = strchr(line, '\r')) != NULL) *p = '\0';
			if ((p = strchr(line, '\n')) != NULL) *p = '\0';

			LL_W_PRINT("Invalid SNAP format on line \"%s\"\n", line);

			_errors++;
			if (_errors >= _max_allowed_errors) {
				LL_E_PRINT("Reached the limit of %lu maximum allowable parse errors\n",
						_max_allowed_errors);
				abort();
			}
		}
	};
};

#endif
