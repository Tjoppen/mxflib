/*! \file   DotFile.cpp
 *  \brief  Utility class for writing GraphViz dot files
 */
/*
 *  Copyright (c) 2006, Philip de Nier, BBC Research and Development
 *
 *  This software is provided 'as-is', without any express or implied warranty.
 *  In no event will the authors be held liable for any damages arising from
 *  the use of this software.
 *
 *  Permission is granted to anyone to use this software for any purpose,
 *  including commercial applications, and to alter it and redistribute it
 *  freely, subject to the following restrictions:
 *
 *    1. The origin of this software must not be misrepresented; you must
 *       not claim that you wrote the original software. If you use this
 *       software in a product, an acknowledgment in the product
 *       documentation would be appreciated but is not required.
 *  
 *    2. Altered source versions must be plainly marked as such, and must
 *       not be misrepresented as being the original software.
 *  
 *    3. This notice may not be removed or altered from any source
 *       distribution.
 */

#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif

 
#include "DotFile.h"
#include <cassert>
#include <cstdio>


using namespace std;
using namespace dot;


DotFile::DotFile(const char* filename)
: _state(START), _nextNodeId(0), _nextClusterId(0) 
{
    _dotFile = fopen(filename, "wb");
    if (_dotFile == 0)
    {
        fprintf(stderr, "Failed to open file");
        throw "Failed to open file";
    }
}

DotFile::~DotFile(void)
{
    fclose(_dotFile);
}
    
void DotFile::startGraph(string id)
{
    assert(_state == START);
    
    string str = "digraph ";
    str += id + " {\n";
        
    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = ELEMENT;
}

void DotFile::endGraph(void)
{
    assert(_state == ELEMENT);

    string str = "}\n";

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 
}

void DotFile::startDefaultAttributes(string elementName)
{
    assert(_state == ELEMENT);

    string str = elementName;
    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 

    _state = DEFAULT_ATTR;
}

void DotFile::endDefaultAttributes(void)
{
    assert(_state == ATTRIBUTES || _state == DEFAULT_ATTR);
    
    
    string str;
    if (_state == ATTRIBUTES)
    {
        str = " ];\n";
    }
    else
    {
        str = ";\n";
    }
    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 

    _state = ELEMENT;
}

void DotFile::startCluster(string id)
{
    assert(_state == ELEMENT);
    
    string str = "subgraph ";
    str += id + " {\n";
        
    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
}

void DotFile::endCluster(void)
{
    assert(_state == ELEMENT);

    string str = "};\n";

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 
}
 
void DotFile::startNode(string id)
{
    assert(_state == ELEMENT);

    string str = id;

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = NODE;
}

void DotFile::endNode(void)
{
    assert(_state == NODE || _state == ATTRIBUTES);

    string str;
    if (_state == NODE)
    {
        str = ";\n";
    }
    else
    {
        str = "];\n";
    }

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = ELEMENT;
}

void DotFile::startEdge(string fromId, string toId)
{
    assert(_state == ELEMENT);

    string str = fromId + " -> ";
    str += toId + " ";

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = EDGE;
}

long DotFile::allocateEdgeSpace(unsigned int size)
{
    assert(_state == ELEMENT);

    long position = ftell(_dotFile);
    assert(position >= 0);
    
    char* space = new char[size];
    memset(space, ' ', size - 1);
    space[size - 1] = '\n';
    fwrite(space, sizeof(char), size, _dotFile);
    delete [] space;
    
    return position;
}

void DotFile::startEdge(long position, string fromId, string toId)
{
    int result = fseek(_dotFile, position, SEEK_SET);
    assert(result == 0);
    
    string str = fromId + " -> ";
    str += toId;

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = EDGE;
}

void DotFile::endEdge(void)
{
    assert(_state == EDGE || _state == ATTRIBUTES);

    string str;
    if (_state == EDGE)
    {
        str = ";\n";
    }
    else
    {
        str = "];\n";
    }

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = ELEMENT;
}

void DotFile::writeAttribute(string id, string value)
{
    assert(_state == ATTRIBUTES || _state == DEFAULT_ATTR || _state == NODE || _state == EDGE);
    
    string str;
    if (_state != ATTRIBUTES)
    {
        str = " [ ";
    }
    else
    {
        str = ", ";
    }
    str += id + " = ";
    str += value;

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile);
    
    _state = ATTRIBUTES;
}

void DotFile::writeAttribute(DotAttribute* attribute)
{
    assert(_state == ATTRIBUTES || _state == DEFAULT_ATTR || _state == NODE || _state == EDGE);
    
    string str;
    if (_state != ATTRIBUTES)
    {
        str = " [ ";
    }
    else
    {
        str = ", ";
    }
    str += attribute->getId() + " = ";
    str += attribute->getValue();

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 

    _state = ATTRIBUTES;
}

void DotFile::writeAttributes(vector<DotAttribute*>& attributes)
{
    assert(_state == ATTRIBUTES || _state == NODE || _state == EDGE);
    
    string str;
    if (_state != ATTRIBUTES)
    {
        str = " [ ";
    }
    else
    {
        str = ", ";
    }
    
    vector<DotAttribute*>::const_iterator iter;
    for (iter = attributes.begin(); iter != attributes.end(); iter++)
    {
        if (iter != attributes.begin())
        {
            str += " ";
        }
        str += (*iter)->getId();
        str += " = ";
        str += (*iter)->getValue();
    }

    fwrite(str.c_str(), sizeof(char), str.length(), _dotFile); 

    _state = ATTRIBUTES;
}

string DotFile::getNextNodeId(void)
{
    char buffer[13];
    sprintf(buffer, "N%lu", _nextNodeId);
    _nextNodeId++;
    
    return buffer;
}

string DotFile::getNextClusterId(void)
{
    char buffer[19];
    sprintf(buffer, "cluster%lu", _nextClusterId);
    _nextClusterId++;
    
    return buffer;
}



DotAttribute::DotAttribute(void)
{}

DotAttribute::~DotAttribute(void)
{}
    
void DotAttribute::setId(string id)
{
    _id = id;
}

void DotAttribute::setValue(string value)
{
    _value = value;
}

string DotAttribute::getId(void)
{
    return _id;
}

string DotAttribute::getValue(void)
{
    return _value;
}



DotObjectAttribute::DotObjectAttribute(void)
: _maxPropertySize(128), _maxPropertyWidth(40), _len(0), _fontSize(10)
{}

DotObjectAttribute::~DotObjectAttribute(void)
{}
  
string DotObjectAttribute::getValue(void)
{
    string value;
    value = "\"{";
    value += _name + "\\n. ";
    
    vector<string>::const_iterator iter;
    for (iter = _properties.begin(); iter != _properties.end(); iter++)
    {
        if (iter != _properties.begin())
        {
            value += "\\n | ";
        }
        else
        {
            value += " | ";
        }
        value += (*iter);
    }
    value += "}\"";
    
    return value;
}

void DotObjectAttribute::setObjectName(string name)
{
    _name = name;
    if (_len < name.length())
    {
        _len = name.length();
    }
}

void DotObjectAttribute::setMaxPropertySize(unsigned int maxPropertySize)
{
    _maxPropertySize = maxPropertySize;
}

void DotObjectAttribute::setMaxPropertyWidth(unsigned int maxPropertyWidth)
{
    _maxPropertyWidth = maxPropertyWidth;
}

void DotObjectAttribute::setFontSize(float fontSize)
{
    _fontSize = fontSize;
}

void DotObjectAttribute::addProperty(string name, string value)
{
    addPropertyValueOrDef(name, value, true);
}

void DotObjectAttribute::addPropertyDef(string name, string type)
{
    addPropertyValueOrDef(name, type, false);
}

void DotObjectAttribute::addPropertyValueOrDef(string name, string typeOrValue, 
    bool isValue)
{
    string property = escapeString(name);
    if (isValue)
    {
        property += " = ";
    }
    else
    {
        property += " : ";
    }
    property += escapeString(typeOrValue);
    
    if (property.size() > _maxPropertySize )
    {
        property.resize(_maxPropertySize - 1 );
        property.resize(_maxPropertySize, '~' );
    }

    size_t newLineIndex = _maxPropertyWidth;
    size_t index = 0;
    bool escape = false;
    while (index < property.length())
    {
        if (index == newLineIndex)
        {
            if (escape)
            {
                index++;
                if (index < property.length())
                {
                    property.insert(index, "\\n");
                }
            }
            else
            {
                property.insert(index, "\\n");
            }
            newLineIndex = index + _maxPropertyWidth + 3;
        }
        else 
        {
            if (property[index] == '\\')
            {
                escape = !escape;
            }
            else
            {
                escape = false;
            }
        }
        index++;
    }

    
    bool done = false;
    int pos = 0;
    while (!done)
    {
        int newPos = property.find( "\\n", pos);
        if (newPos == -1)
        {
            done = true;
            if ((property.length() - pos ) > _len)
            {
                _len = (int)property.length() - pos;
            }
        }
        else
        {
            if ((unsigned int)(newPos - pos) > _len)
            {
                _len = newPos - pos;
            }
            pos = newPos + 1;
        }
    }
    
    _properties.push_back(property);
}

string DotObjectAttribute::getDisplayWidth(float fixedPitch)
{
    double nodeWidth;
    if (fixedPitch <= 0.0)
    {
        // conservative estimate
        // 1 pt == 1/72 inch and assuming font aspect ratio == 1
        nodeWidth = _len * _fontSize / 72.0;
    }
    else
    {
        // pitch is number of characters per inch
        nodeWidth = _len * 1.0 / fixedPitch;
    }
    
    char buffer[7];
    sprintf(buffer, "%6.2f", static_cast<float>(nodeWidth));
    return buffer;
}

string DotObjectAttribute::escapeString(string value)
{
    size_t index = 0;
    while (index < value.size())
    {
        if (value[index] == '\n')
        {
            value.erase(index, 1);
            value.insert(index, "\\n");
            index += 1;
        }
        else if (value[index] == '"' ||
            value[index] == '\\' ||
            value[index] == '<' ||
            value[index] == '>' ||
            value[index] == '{' ||
            value[index] == '}' ||
            value[index] == '=' ||
            value[index] == '|' )
        {
            value.insert(index, "\\");
            index += 1;
        }
        // tab etc. become spaces
        else if (isspace((unsigned char)value[index]))
        {
            value[index] = ' ';
        }
        // replace non-printable or control characters with "?"
        else if (!isprint((unsigned char)value[index]) || 
            iscntrl((unsigned char)value[index]))
        {
            value[index] = '?';
        }
        index++;
    }
    
    return value;
}



void test(string filename)
{
    DotFile testFile(filename.c_str());
    
    testFile.startGraph("MXF");
    
    // setup default attributes for graph, node and edge
    
    testFile.startDefaultAttributes("graph");
    testFile.writeAttribute("concentrate", "false");
    testFile.endDefaultAttributes();
    
    testFile.startDefaultAttributes("node");
    testFile.writeAttribute("fontname", "Courier");
    testFile.writeAttribute("fontsize", "12");
    testFile.writeAttribute("shape", "record");
    testFile.endDefaultAttributes();
    
    testFile.startDefaultAttributes("edge");
    testFile.writeAttribute("color", "black");
    testFile.writeAttribute("style", "solid");
    testFile.writeAttribute("weight", "1");
    testFile.endDefaultAttributes();

    
    // create a cluster containing 2 nodes and 2 edges 
    
    testFile.startCluster(testFile.getNextClusterId());
    
    // MasterPackage node
    string n1 = testFile.getNextNodeId();
    testFile.startNode(n1);
    DotObjectAttribute oa1;
    oa1.setId("label");
    oa1.setObjectName("MasterPackage");
    oa1.addProperty("PackageID", "060c2b34020511010104100013000000-00310612-ec10-0195-060e2b347f7f2a80");
    oa1.addProperty("LastModified", "2002-02-22 18:06:49.00");
    oa1.addProperty("CreationTime", "2002-02-22 18:06:49.00");
    testFile.writeAttribute(&oa1);
    testFile.endNode();
    
    // TimelineTrack node
    string n2 = testFile.getNextNodeId();
    testFile.startNode(n2);
    DotObjectAttribute oa2;
    oa2.setId("label");
    oa2.setObjectName("TimelineTrack");
    oa2.addProperty("TrackID", "1");
    oa2.addProperty("TrackName", "Video Timeline");
    testFile.writeAttribute(&oa2);
    testFile.endNode();
    
    // allocate space for an edge which we will fill in later
    long pos = testFile.allocateEdgeSpace();
    
    // edge MasterPackage->TimelineTrack
    testFile.startEdge(n1, n2);
    testFile.writeAttribute("weight", "5");
    testFile.endEdge();
    
    testFile.endCluster();

    
    testFile.endGraph();


    // edge TimelineTrack->MasterPackage
    // using space allocated previously
    testFile.startEdge(pos, n2, n1);
    testFile.writeAttribute("label", "1");
    testFile.writeAttribute("weight", "5");
    testFile.endEdge();
}



