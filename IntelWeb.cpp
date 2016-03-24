#include "IntelWeb.h"
using namespace std;

int IntelWeb::findPrevalence(string& line)
{
	int prevalences = 0;
	DiskMultiMap::Iterator it = interactions.search(line);

	while (it.isValid())
	{
		prevalences++;
		++it;
	}

	it = revInteractions.search(line);

	while (it.isValid())
	{
		prevalences++;
		++it;
	}

	return prevalences;
}

IntelWeb::IntelWeb() {}

IntelWeb::~IntelWeb()
{
	close();
}


bool IntelWeb::createNew(const std::string& filePrefix, unsigned int maxDataItems)
{
	close(); // close open files first
	int hashTabSize = maxDataItems / (.75);
	if (!interactions.createNew(filePrefix + "_forward.dat", hashTabSize) ||
		!revInteractions.createNew(filePrefix + "_reverse.dat", hashTabSize))
		return false;
	return true;
}

bool IntelWeb::openExisting(const std::string & filePrefix)
{
	// close any current files
	interactions.close();
	revInteractions.close();
	
	if (!interactions.openExisting(filePrefix + "_forward.dat") ||
		!revInteractions.openExisting(filePrefix + "_reverse.dat"))
		return false;
	return true;
}

void IntelWeb::close()
{
	interactions.close();
	revInteractions.close();
}

bool IntelWeb::ingest(const std::string & telemetryFile)
{ 
	ifstream ifs(telemetryFile);
	if (!ifs)
	{
		return false;
	}

	string readLine;
	while (getline(ifs, readLine))
	{
		istringstream iss(readLine);
		string from, to, context;
		iss >> from >> to >> context;
		interactions.insert(from, to, context);
		revInteractions.insert(to, from, context);
	}
	return true;
}

unsigned int IntelWeb::crawl(const std::vector<std::string>& indicators,
	unsigned int minPrevalenceToBeGood,
	std::vector<std::string>& badEntitiesFound,
	std::vector<InteractionTuple>& badInteractions
	)
{ 
	badInteractions.clear();
	badEntitiesFound.clear();
	unsigned int nMaliciousEntities = 0;
	queue<string> entitiesToCheck;
	set<string> entitiesChecked;
	set<InteractionTuple> badInters;
	map<string, int> prevalenceMap;
	vector<string>::const_iterator it = indicators.begin();

	while (it != indicators.end())
	{
		entitiesToCheck.push(*it);
		nMaliciousEntities++;
		it++;
	}

	while (!entitiesToCheck.empty())
	{
		string toCheck = entitiesToCheck.front();
		entitiesToCheck.pop();
		DiskMultiMap::Iterator it = interactions.search(toCheck);
		while (it.isValid())
		{
			MultiMapTuple mmt = *it;
			InteractionTuple iTup;
			iTup.context = mmt.context;
			iTup.from = mmt.key;
			iTup.to = mmt.value;
			if (iTup.from != toCheck)
			{
				++it;
				continue;
			}
			badInters.insert(iTup);
			map<string, int>::iterator mIt = prevalenceMap.find(iTup.to);
			int prevalence;
			if (mIt == prevalenceMap.end())
			{
				prevalence = findPrevalence(iTup.to);
				prevalenceMap[iTup.to] = prevalence;
			}
			else
				prevalence = (*mIt).second;

			if (prevalence < minPrevalenceToBeGood)
			{
				set<string>::iterator checkChecked = entitiesChecked.find(iTup.to);
				if (checkChecked == entitiesChecked.end())
				{
					entitiesToCheck.push(iTup.to);
					nMaliciousEntities++;
				}
			}
			++it;
		}
	}
	return nMaliciousEntities;
}

bool IntelWeb::purge(const std::string & entity)
{
	bool purged = false;
	queue<string> forwardVals;
	queue<string> revVals;
	queue<MultiMapTuple> deleteFVals;
	queue<MultiMapTuple> deleteRevVals;
	DiskMultiMap::Iterator it = interactions.search(entity);
	
	if (it.isValid())
	{
		purged = true;
		while (it.isValid())
		{
			MultiMapTuple mmt = *it;
			if (mmt.key == entity) {
				forwardVals.push(mmt.value);
				deleteFVals.push(mmt);
			}
			++it;
		}
	}

	it = revInteractions.search(entity);
	if (it.isValid())
	{
		purged = true;
		while (it.isValid())
		{
			MultiMapTuple mmt = *it;
			if (mmt.key == entity) {
				revVals.push(mmt.value);
				deleteRevVals.push(mmt);
			}
			++it;
		}
	}

	while (!forwardVals.empty()) {
		string toCheck = forwardVals.front();
		forwardVals.pop();
		it = revInteractions.search(toCheck);
		while (it.isValid()) {
			MultiMapTuple mmt = *it;
			if (mmt.value == entity) {
				deleteRevVals.push(mmt);
			}
			++it;
		}
	}
	while (!revVals.empty()) {
		string toCheck = revVals.front();
		revVals.pop();
		it = interactions.search(toCheck);
		while (it.isValid()) {
			MultiMapTuple mmt = *it;
			if (mmt.value == entity) {
				deleteFVals.push(mmt);
			}
			++it;
		}
	}
	while (!deleteFVals.empty()) {
		MultiMapTuple mmt = deleteFVals.front();
		deleteFVals.pop();
		interactions.erase(mmt.key, mmt.value, mmt.context);
	}
	while (!deleteRevVals.empty()) {
		MultiMapTuple mmt = deleteRevVals.front();
		deleteRevVals.pop();
		interactions.erase(mmt.key, mmt.value, mmt.context);
	}
	return purged;
}

bool operator<(const InteractionTuple& T1, const InteractionTuple& T2)
{
	if (T1.context < T2.context)
		return true;
	else if (T1.context > T2.context)
		return false;
	else if (T1.from < T2.from)
		return true;
	else if (T1.from > T2.from)
		return false;
	else if (T1.to < T2.to)
		return true;
	else if (T1.to > T2.to)
		return false;
	else
		return false;
}
