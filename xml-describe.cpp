#include <iostream>
#include <iterator>
#include <string>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>

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

static void add_options(po::options_description &desc, std::string *tag)
{
	desc.add_options()
		("help,?", "emit this help message")
		("tag,t", po::value<std::string>(tag), "describe (only) this tag's data")
		;
}

//
// illustrates how to use property-tree to parse XML
//
int main(int argc, const char* argv[])
{
	std::string tag("");
	po::options_description desc("Arguments");
	add_options(desc, &tag);

	po::variables_map vm;
	po::store(po::parse_command_line(argc, argv, desc), vm);
	po::notify(vm);

	if (vm.count("help")) {
		std::cerr << argv[0] << "-" << GIT_VERSION << std::endl;
		std::cerr << "Describes an XML document" << std::endl;
		std::cerr << desc << std::endl;
		return 1;
	}

	// read all of stdin
	std::istreambuf_iterator<char> begin(std::cin), end;
	std::string s(begin, end);
	std::istringstream is(s);

	// convert xml to property-tree
	boost::property_tree::ptree p;
	int flags = boost::property_tree::xml_parser::trim_whitespace;
	boost::property_tree::read_xml(is, p, flags);
	
	std::string ret;

	if (tag.length())
	{
		boost::property_tree::ptree pt;
		pt = p.get_child(tag);
		if (pt.size()) {
			ret = tag;
			ret += describe(pt, 1, "\n");
			std::string pts = pt.get_value<std::string>();
			if (pts.length())
				ret = ret + " = \"" + pts + "\"";
		}
		else
		{
			ret = tag;
			ret += " = \"";
			ret += p.get<std::string>(tag);
			ret += "\"";
		}
	}
	else
		ret = describe(p);

	std::cout << ret << std::endl;

	return 0;
}
