//
// Parses XML into a corresponding MAVLink binary message
//
#include <iostream>
#include <iterator>
#include <string>
#include <map>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

#include <mavlink/common/mavlink.h>

static const std::string xmlattr("<xmlattr>");


std::string describe(boost::property_tree::ptree &p, int indent = 0, std::string term = ", ")
{
	std::string ret("");

	// describe attributes first, even if they aren't the first element
	boost::property_tree::ptree pc;
	pc = p.get_child(xmlattr, pc);
	if (pc.size())
	{
		ret += "(";
		ret += describe(pc, 0, ", ");
		ret.pop_back();
		ret.pop_back();
		ret += ")\n";
	}

	for (auto i = p.begin(); i != p.end(); ++i)
	{
		if (i->first.data() == xmlattr)
			continue; // (already described above)
		
		ret.append(indent, ' ');
		ret += i->first.data();

		if (i->second.size()) // describe children, if any
			ret += describe(i->second, indent + 1, "\n");

		if (i->second.data().length())
		{
			// describe this element
			if (ret.back() == '\n')
			{
				ret.append(indent + 1, ' ');
				ret += "...";
			}
			ret += " = ";
			ret = ret + "\"" + i->second.data() + "\"";
			ret += term;
		}
	}
	return ret;
}



#include <boost/program_options.hpp>

namespace po = boost::program_options;

static bool summarize = false;
static std::string msg("");

static void add_options(po::options_description &desc)
{
	desc.add_options()
		("help,?", "emit this help message")
		("all,a", po::bool_switch(&summarize), "describe all known messages")
		("msg,m", po::value<std::string>(&msg), "describe only this message")
		;
}

static std::map<std::size_t, std::string> mavlink_type;
static std::map<std::size_t, mavlink_message_info_t> mavlink_message_info;
static std::map<std::string, std::size_t> mavlink_message_id = MAVLINK_MESSAGE_NAMES;

std::string describe(mavlink_message_info_t *info)
{
	std::stringstream o;
	
	o << "msgid = " << std::to_string(info->msgid) << std::endl;
	o << "name = " << info->name << std::endl;
	o << "num_fields = " << std::to_string(info->num_fields) << std::endl;

	for (std::size_t n = 0; n < info->num_fields; n++)
	{
		o << "fields[" << std::to_string(n) << "]" << std::endl;

		mavlink_field_info_t *f = &info->fields[n];
		o << "    .name = " << f->name << std::endl;
		o << "    .print_format = ";
		if (f->print_format)
			o << f->print_format << std::endl;
		else
			o << "(null)" << std::endl;
		o << "    .type = " << mavlink_type[f->type] << " (" << std::to_string(f->type) << ")" << std::endl;
		o << "    .array_length = " << std::to_string(f->array_length) << std::endl;
		o << "    .wire_offset = " << std::to_string(f->wire_offset) << std::endl;
		o << "    .structure offset = " << std::to_string(f->structure_offset) << std::endl;
	}
	return o.str();
}


int main(int argc, const char* argv[])
{
	std::ostream &o(std::cerr);
	
	po::options_description desc("Arguments");
	add_options(desc);

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		o << argv[0] << "-" << GIT_VERSION << std::endl;
		o << "Describes one or more MAVLink messages" << std::endl;
		o << desc << std::endl;
		return 1;
	}

	{
		// build mavlink_type map
		mavlink_type[MAVLINK_TYPE_CHAR] = "MAVLINK_TYPE_CHAR";
		mavlink_type[MAVLINK_TYPE_UINT8_T] = "MAVLINK_UINT8_T";
		mavlink_type[MAVLINK_TYPE_INT8_T] = "MAVLINK_TYPE_INT8_T";
		mavlink_type[MAVLINK_TYPE_UINT16_T] = "MAVLINK_TYPE_UINT16_T";
		mavlink_type[MAVLINK_TYPE_INT16_T] = "MAVLINK_INT16_T";
		mavlink_type[MAVLINK_TYPE_UINT32_T] = "MAVLINK_TYPE_UINT32_T";
		mavlink_type[MAVLINK_TYPE_INT32_T] = "MAVLINK_INT32_T";
		mavlink_type[MAVLINK_TYPE_UINT64_T] = "MAVLINK_TYPE_UINT64_T";
		mavlink_type[MAVLINK_TYPE_INT64_T] = "MAVLINK_INT64_T";
		mavlink_type[MAVLINK_TYPE_FLOAT] = "MAVLINK_TYPE_FLOAT";
		mavlink_type[MAVLINK_TYPE_DOUBLE] = "MAVLINK_TYPE_DOUBLE";
	}
	{
		// build mavlink_message_info map
		const mavlink_message_info_t msg_info[] = MAVLINK_MESSAGE_INFO;
		for (std::size_t n = 0; n < mavlink_message_id.size(); n++)
			mavlink_message_info[msg_info[n].msgid] = msg_info[n];
	}

	if (msg.length())
	{
		// find a message based on its text name
		std::size_t id = mavlink_message_id[msg];
		// o << "\"" << msg << "\" maps to message id: "
		//   << std::to_string(id) << std::endl;
		mavlink_message_info_t* msg_info = &mavlink_message_info[id];
		o << describe(msg_info);
		o << std::endl;
	}

	if (summarize)
	{
		// summarize all recognized messages
		for (auto &i : mavlink_message_id)
		{
			// pretty-print some test output
			mavlink_message_info_t* msg_info = &mavlink_message_info[i.second];
			o << describe(msg_info);
			o << std::endl;
		}
	}
	return 0;
}
