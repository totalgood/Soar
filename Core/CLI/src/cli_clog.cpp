/////////////////////////////////////////////////////////////////
// log command file.
//
// Author: Jonathan Voigt, voigtjr@gmail.com
// Date  : 2004
//
/////////////////////////////////////////////////////////////////

#include <portability.h>

#include "cli_CommandLineInterface.h"

#include <fstream>

#include <assert.h>

#include "cli_Commands.h"
#include "cli_CommandData.h"

#include "sml_Names.h"
#include "sml_Events.h"

using namespace cli;
using namespace sml;

bool CommandLineInterface::ParseCLog(std::vector<std::string>& argv) {
	Options optionsData[] = {
		{'a', "add",		OPTARG_NONE},
		{'A', "append",		OPTARG_NONE},
		{'c', "close",		OPTARG_NONE},
		{'d', "disable",	OPTARG_NONE},
		{'e', "existing",	OPTARG_NONE},
		{'d', "off",		OPTARG_NONE},
		{'q', "query",		OPTARG_NONE},
		{0, 0, OPTARG_NONE}
	};

	eLogMode mode = LOG_NEW;

	for (;;) {
		if (!ProcessOptions(argv, optionsData)) return false;
		if (m_Option == -1) break;

		switch (m_Option) {
			case 'a':
				mode = LOG_ADD;
				break;
			case 'c':
			case 'd':
			case 'o':
				mode = LOG_CLOSE;
				break;
			case 'e':
			case 'A':
				mode = LOG_NEWAPPEND;
				break;
			case 'q':
				mode = LOG_QUERY;
				break;
			default:
				return SetError(kGetOptError);
		}
	}
	
	switch (mode) {
		case LOG_ADD:
			{
				std::string toAdd;
				// no less than one non-option argument
				if (m_NonOptionArguments < 1) {
					SetErrorDetail("Provide a string to add.");
					return SetError(kTooFewArgs);
				}

				// move to the first non-option arg
				std::vector<std::string>::iterator iter = argv.begin();
				for (int i = 0; i < (m_Argument - m_NonOptionArguments); ++i) ++iter;

				// combine all args
				while (iter != argv.end()) {
					toAdd += *iter;
					toAdd += ' ';
					++iter;
				}
				return DoCLog(mode, 0, &toAdd);
			}

		case LOG_NEW:
			// no more than one argument, no filename == query
			if (m_NonOptionArguments > 1) {
				SetErrorDetail("Filename or nothing expected, enclose filename in quotes if there are spaces in the path.");
				return SetError(kTooManyArgs);
			}
			if (m_NonOptionArguments == 1) return DoCLog(mode, &argv[1]);
			break; // no args case handled below

		case LOG_NEWAPPEND:
			// exactly one argument
			if (m_NonOptionArguments > 1) {
				SetErrorDetail("Filename expected, enclose filename in quotes if there are spaces in the path.");
				return SetError(kTooManyArgs);
			}

			if (m_NonOptionArguments < 1) {
				SetErrorDetail("Please provide a filename.");
				return SetError(kTooFewArgs);
			}
			return DoCLog(mode, &argv[1]);

		case LOG_CLOSE:
		case LOG_QUERY:
			// no arguments
			if (m_NonOptionArguments) {
				SetErrorDetail("No arguments when querying log status.");
				return SetError(kTooManyArgs);
			}
			break; // no args case handled below

		default:
			return SetError(kInvalidOperation);
	}

	// the no args case
	return DoCLog(mode);
}

bool CommandLineInterface::DoCLog(const eLogMode mode, const std::string* pFilename, const std::string* pToAdd, bool silent) {
	std::ios_base::openmode openmode = std::ios_base::out;

 	switch (mode) {
		case LOG_NEWAPPEND:
			openmode |= std::ios_base::app;
			// falls through

		case LOG_NEW:
			if (!pFilename) break; // handle as just a query

			if (m_pLogFile) {
				SetErrorDetail("Currently logging to " + m_LogFilename);
				return SetError(kLogAlreadyOpen);
			}

            {
                std::string filename = *pFilename;

			    m_pLogFile = new std::ofstream(filename.c_str(), openmode);
			    if (!m_pLogFile) {
				    SetErrorDetail("Failed to open " + filename);
				    return SetError(kLogOpenFailure);
			    }

			    m_LogFilename = filename;
            }
			break;

		case LOG_ADD:
			if (!m_pLogFile) return SetError(kLogNotOpen);
			(*m_pLogFile) << *pToAdd << std::endl;
			return true;

		case LOG_CLOSE:
			if (!m_pLogFile) return SetError(kLogNotOpen);

			delete m_pLogFile;
			m_pLogFile = 0;
			m_LogFilename.clear();
			break;

		case LOG_QUERY:
			break;
		default: assert(false);
	}

	if (!silent) {
		LogQuery();
	}
	return true;
}

void CommandLineInterface::LogQuery() {
	if (m_RawOutput) {
		m_Result << "Log file ";
		if (IsLogOpen()) {
			m_Result << "'" + m_LogFilename + "' open.";
		} else {
			m_Result << "closed.";
		}

	} else {
		const char* setting = IsLogOpen() ? sml_Names::kTrue : sml_Names::kFalse;
		AppendArgTagFast(sml_Names::kParamLogSetting, sml_Names::kTypeBoolean, setting);

		if (m_LogFilename.size()) AppendArgTagFast(sml_Names::kParamFilename, sml_Names::kTypeString, m_LogFilename);
	}
}

bool CommandLineInterface::IsLogOpen() {
	return m_pLogFile ? true : false;
}