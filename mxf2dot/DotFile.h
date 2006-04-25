/*! \file   DotFile.h
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

#ifndef __DOTFILE_H__
#define __DOTFILE_H__

#include <string>
#include <vector>


namespace dot
{

class DotAttribute;

class DotFile
{
public:
    DotFile(const char* filename);
    ~DotFile(void);
    
    void startGraph(std::string id);
    void endGraph(void);
    
    void startDefaultAttributes(std::string elementName);
    void endDefaultAttributes(void);
    
    void startCluster(std::string id);
    void endCluster(void);
 
    void startNode(std::string id);
    void endNode(void);
    
    void startEdge(std::string fromId, std::string toId);
    void endEdge(void);
    // allocate space for writing an edge later using startEdge(position... 
    long allocateEdgeSpace(unsigned int size = 60);
    void startEdge(long position, std::string fromId, std::string toId);

    void writeAttribute(std::string id, std::string value);
    void writeAttribute(DotAttribute* attribute);
    void writeAttributes(std::vector<DotAttribute*>& attributes);
    
    
    std::string getNextNodeId(void);
    std::string getNextClusterId(void);
    
    
    void test(std::string filename);
    
private:
    enum DotState
    {
        START,
        ELEMENT,
        DEFAULT_ATTR,
        NODE,
        EDGE,
        ATTRIBUTES,
        END
    };
    
    FILE*       _dotFile;
    DotState    _state;
    long        _nextNodeId;
    long        _nextClusterId;
};


class DotAttribute
{
public:
    DotAttribute(void);
    virtual ~DotAttribute(void);
    
    void setId(std::string id);
    void setValue(std::string value);
    std::string getId(void);
    virtual std::string getValue(void);

protected:
    std::string _id;
    std::string _value;
};


class DotObjectAttribute : public DotAttribute
{
public:
    DotObjectAttribute(void);
    virtual ~DotObjectAttribute(void);
  
    virtual std::string getValue(void);
    
    void setMaxPropertySize(unsigned int maxPropertySize);
    void setMaxPropertyWidth(unsigned int maxPropertyWidth);
    void setObjectName(std::string name);
    void addProperty(std::string name, std::string value);
    void addPropertyDef(std::string name, std::string type);
    // returns the width in inches
    std::string getDisplayWidth(float fixedPitch = -1.0);
    void setFontSize(float fontSize);
    
protected:
    void addPropertyValueOrDef(std::string name, std::string typeOrValue, bool isValue);
    std::string escapeString(std::string value);
    
    std::string                 _name;
    std::vector<std::string>    _properties;
    unsigned int                _maxPropertySize;
    unsigned int                _maxPropertyWidth;
    unsigned int                _len;
    float                       _fontSize;
};
    

};

#endif
