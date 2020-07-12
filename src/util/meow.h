#ifndef _MEOW_H
#define _MEOW_H

#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#define isNumber(x) (x.size() > 0 && (isdigit(x.at(0))))

#define isTmp(x) (x.size() > 1 && (x.at(0) == '%') && (isdigit(x.at(1))))

#define isGlobal(x) (x.size() > 1 && (x.at(0) == '@'))

#define isVar(x) (x.size() > 1 && (isalpha(x.at(1)))

//和python的format的使用方法一致 FORMAT("{}:{}", a, b) => "a:b";
#define FORMAT meow::Meow::format

//字符串->int数字
#define A2I meow::Meow::a2i

//int数字->字符串
#define I2A meow::Meow::i2a

//异常输出
#define eprintf(...)            \
    do {        \
        std::cerr << FORMAT(__VA_ARGS__); \
    } while(0)

//警告信息
#define panic(...)                  \
    do {                            \
        eprintf("panic at {}:{} \n", __FILE__, __LINE__);   \
        eprintf("\t");              \
        eprintf(__VA_ARGS__);        \
        eprintf("\n");    \
		/*throw;*/	\
        /*exit(-1);*/        \
    } while(0)


//这个可以不用管，直接用上面的宏就可以
namespace meow {
	class Meow {
	public:
		static std::string i2a(int num) {
			return std::to_string(num);
		}

		static int a2i(std::string str) {
			return atoi(str.c_str());
		}

		template <typename T>
		static std::string toString(T t) {
			std::stringstream out;
			out << t;
			return out.str();
		}

		template <typename T>
		static std::vector<std::string>& expandToString(std::vector<std::string>& out, T t) {
			out.emplace_back(toString(t));
			return out;
		}

		template <typename T, typename... Types>
		static std::vector<std::string>& expandToString(std::vector<std::string>& out, T t, Types... types) {
			out.emplace_back(toString(t));
			expandToString(out, types...);
			return out;
		}

		static std::string format(const char* fmt) {
			return toString(fmt);
		}

		template <typename... Args>
		static std::string format(const char* fmt, Args... args) {
			auto string_buffer = std::vector<std::string>();
			auto strings = expandToString(string_buffer, args...);
			std::stringstream out;
			int i = 0;
			while (*fmt != '\0') {
				if (*fmt == '{') {
					fmt++;
					std::stringstream cmd_buffer;
					while (*fmt != '\0' && *fmt != '}') {
						cmd_buffer << *fmt;
						fmt++;
					}
					if (*fmt == '\0') {
						panic("wrong format");
					}
					else {
						if (i >= strings.size()) {
							panic("unexpected argument.");
						}
						// TODO(zyc): add format control support.
						out << strings[i++];
					}
					fmt++;
					continue;
				}
				out << *fmt;
				fmt++;
			}
			if (i != strings.size()) {
				panic("redundant argument.");
			}
			return out.str();
		}
	};
}

//change x into "x"
#define TOSTR(x) (#x)

//输出带有msg的错误信息
#define ERROR_IN_COMPILER(msg) do { std::cerr<< "The error occurred in : " << __FILE__ \
<< "; in FUNCTION: " << __FUNCTION__ \
<< "; at line: " << __LINE__ \
<< "; msg: " << (msg) \
<<std::endl; \
}while (0)

//输出错误信息,，附带错误的位置和文件
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d)\n", \
    __LINE__, __FILE__, errno); /*exit(1);*/ } while (0)

//输出错误信息，附带错误的位置和文件以及错误的信息
#define FATAL_MSG(str) do { fprintf(stderr, "%s, Error at line %d, file %s (%d)\n", \
    str, __LINE__, __FILE__, errno); } while (0)

//输出警告信息，附带错误的位置和文件以及错误的信息
#define WARN_MSG(str) do { fprintf(stderr, "%s, Warn at line %d, file %s (%d)\n", \
    str, __LINE__, __FILE__, errno); } while (0)

#endif //_MEOW_H