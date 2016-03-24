#include "DiskMultiMap.h"


/////// ITERATOR /////////////////////////////////////////////////////////
DiskMultiMap::Iterator::Iterator()
{
	location = 0;
}

DiskMultiMap::Iterator::Iterator(BinaryFile::Offset address, DiskMultiMap* dmm)
	: m_dmm(dmm), location(address)
{
	m_dmm = dmm;
	location = address;
}

bool DiskMultiMap::Iterator::isValid() const
{
	if (location != 0)
		return true;
	else
		return false;
}

DiskMultiMap::Iterator& DiskMultiMap::Iterator::operator++()
{
	if (isValid() == false)
		return *this;
	Node n;
	m_dmm->bf.read(n, location);
	if (n.next == 0)
		location = 0;
	else
		location = n.next;
	return *this;
}

MultiMapTuple DiskMultiMap::Iterator::operator*()
{
	MultiMapTuple mmt;
	if (isValid() == false)
		return mmt;
	Node n;
	m_dmm->bf.read(n, location);
	mmt.key = n.key;
	mmt.value = n.value;
	mmt.context = n.context;
	return mmt;
}
///////////////////////////////////////////////////////////////////////////////

////////// DISKMULTIMAP ///////////////////////////////////////////////////////
DiskMultiMap::DiskMultiMap()
{}

DiskMultiMap::~DiskMultiMap()
{
	close();
}

unsigned int DiskMultiMap::strHashFunction(const std::string& hashThis)
{
	std::hash<string> strHash;
	Header h;
	bf.read(h, 0);
	unsigned int hashVal = strHash(hashThis);
	int split = h.head;
	BinaryFile::Offset bucketLoc = hashVal % split;
	return bucketLoc + sizeof(Header);
}

bool DiskMultiMap::createNew(const std::string & filename, unsigned int numBuckets)
{
	close();
	if (bf.createNew(filename) == false)
		return false;
	nkeys = numBuckets;
	Header h;
	h.head = numBuckets;
	h.freeList = 0;

	if (bf.write(h, 0) == false)
			return false;
	BinaryFile::Offset buckets = 0;
	for (int k = 0; k < numBuckets; k++)
	{
		if (bf.write(buckets, bf.fileLength()) == false)
			return false;
	}

	return true;
}

bool DiskMultiMap::openExisting(const std::string & filename)
{
	close(); // close current data file to save data
	return bf.openExisting(filename); // return success of method
}

void DiskMultiMap::close()
{
	bf.close(); // closes the data file
}

BinaryFile::Offset DiskMultiMap::getNode()
{
	Header h;
	BinaryFile::Offset newNode;
	
	bf.read(h, 0);
	if (h.freeList == 0)
	{
		newNode = bf.fileLength();
		return newNode;
	}
	else
	{
		newNode = h.freeList;
		Node n;
		bf.read(n, h.freeList);
		h.freeList = n.next;
		bf.write(h, 0);
		return newNode;
	}
}

bool DiskMultiMap::insert(const std::string& key, const std::string& value,
	const std::string& context)
{
	if (key.size() > MAX_DATA_LENGTH || value.size() > MAX_DATA_LENGTH || context.size() > MAX_DATA_LENGTH)
		return false;

	BinaryFile::Offset nodeAddress = getNode();
	Node newNode;
	strcpy_s(newNode.key, key.c_str());
	strcpy_s(newNode.value, value.c_str());
	strcpy_s(newNode.context, context.c_str());
	
	BinaryFile::Offset nodePtrAddy;
	BinaryFile::Offset bucketAddress = strHashFunction(key);
	bf.read(nodePtrAddy, bucketAddress);
	bf.write(nodeAddress, bucketAddress);
	bf.write(newNode, nodeAddress);
	return true;
}

DiskMultiMap::Iterator DiskMultiMap::search(const std::string & key)
{
	BinaryFile::Offset temp;
	BinaryFile::Offset bucketLoc = strHashFunction(key);
	
	bf.read(temp, bucketLoc);
	if (temp == 0)
	{
		Iterator i;
		return i;
	}
	else {
		Iterator i(temp, this);
		return i;
	}
}

int DiskMultiMap::erase(const std::string& key, const std::string& value, const std::string& context)
{
	int numNodesDeleted = 0;
	BinaryFile::Offset nodeLoc;
	BinaryFile::Offset bucketLoc = strHashFunction(key);
	BinaryFile::Offset none = 0;
	bf.read(nodeLoc, bucketLoc);
	if (nodeLoc == 0)
		return 0;
	else 
	{
		Node n;
		BinaryFile::Offset oldNext;
		BinaryFile::Offset current;
		BinaryFile::Offset prev;

		Header h;
		bf.read(h, 0);
		current = nodeLoc;
		bf.read(n, current);
		prev = 0;
		while (current != 0)
		{
			bf.read(n, current);
			bf.read(h, 0);
			oldNext = n.next;//why does this not return 0?
			if (n.key == key && n.value == value && n.context == context)
			{

				std::cerr << "Erasing node at location: " << current << endl;
				if (prev == 0) //the first one after the bucket
				{
					BinaryFile::Offset newNodeLoc = n.next;
					bf.write(newNodeLoc, bucketLoc);
				}
				else
				{
					Node previousNewLink;
					bf.read(previousNewLink, prev);
					previousNewLink.next = n.next;
					bf.write(previousNewLink, prev);
				}
				
				if (h.freeList == 0)
				{

					h.freeList = current;
					n.next = none;
					bf.write(n, current);
					bf.write(h, 0);
				}
				else 
				{
					n.next = h.freeList;
					h.freeList = current;
					bf.write(n, current);
					bf.write(h, 0);
				}
				if (prev != 0)
					prev = current;
				
				current = oldNext;
				numNodesDeleted++;

			}
			else 
			{

				prev = current;
				current = oldNext;
			}
		} 

	}
	return numNodesDeleted;
}
//////////////////////////////////////////////////////////////////////////////
