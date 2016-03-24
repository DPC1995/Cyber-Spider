#include <sstream>
#include <map>
#include <set>
#include <queue>
#include <iostream>
#include <string>
#include <vector>

class IntelWeb
{
public:
	IntelWeb();
	~IntelWeb();
	bool createNew(const std::string& filePrefix, unsigned int maxDataItems);
	bool openExisting(const std::string& filePrefix);
	void close();
	bool ingest(const std::string& telemetryFile);
	unsigned int crawl(const std::vector<std::string>& indicators,
		unsigned int minPrevalenceToBeGood,
		std::vector<std::string>& badEntitiesFound,
		std::vector<InteractionTuple>& badInteractions
		);
	bool purge(const std::string& entity);

private:
	// Your private member declarations will go here
	DiskMultiMap interactions; //Interactions
	DiskMultiMap revInteractions; //Reverse Interactions
	int findPrevalence(string& line);

};

#endif // INTELWEB_H_
