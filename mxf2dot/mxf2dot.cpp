/*! \file   mxf2dot.cpp
 *  \brief  Utility to dump an MXF file as a GraphViz dot file
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

/*
 * Converts an MXF file to a GraphViz (http://www.graphviz.org) dot file.
 *
 * The Graphviz web page lists the tools that can be used to view the dot file,
 * as well as the images formats that the dot file can be converted to.
 * 
 * The zgrviewer (http://zvtm.sourceforge.net) was found to work well for MXF 
 * files because it has easy to use pan and zoom for quick navigation. The AAF 
 * SDK (http://aaf.sourceforge.net) includes a modified version of zgrviewer for 
 * viewing AAF files. Adode's SVG Viewer to view the dot file after converting 
 * it to the SVG format.
 * 
 *
 * The graph represented by the dot file shows the metadata object structure 
 * contained in a closed and complete header or complete footer partition 
 * (unless the -p option is used to show the metadata in a specific partition). 
 * The Preface object is the root of the object tree.
 *  
 * Strong references are shown in black, weak references 
 * (eg. Preface::PrimaryPackage) in blue and source references (eg. SourceClip) 
 * in orange. References contained in arrays or batches are indexed with their 
 * position number.
 *
 * 
 * Philip de Nier, BBC Research and Development
 * 
 */
 

#ifdef _MSC_VER
#pragma warning (disable:4786)
#endif


#include "DotFile.h"
#include <mxflib/mxflib.h>

#include <vector>
#include <cassert>
#include <stdarg.h>


#ifdef COMPILED_DICT
#include <mxflib/dict.h>
#endif

using namespace std;
using namespace dot;
using namespace mxflib;


// Debug flag for KLVLib
int Verbose = 0;

static bool mxf2dotDebugFlag = false;
static bool mxfLibDebugFlag = false;


// Note: fonts behave differently in different viewers so if you want to change
// these setting then try it out on a number of viewers
static const char* FONT_NAME = "Courier";
static const char* FONT_SIZE = "12";
static const float FONT_FIXED_PITCH = 8.0; // designed for optimal viewing in aaf/zgrviewer


// MXFLib messages

#ifdef MXFLIB_DEBUG
void mxflib::debug(const char* format, ...)
{
    if (!mxfLibDebugFlag)
    {
        return;
    }

    va_list args;

    va_start(args, format);
    fprintf(stdout, "MXFLib debug: ");
    vfprintf(stdout, format, args);
    va_end(args);
}
#endif

void mxflib::warning(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stdout, "MXFLib warning: ");
    vfprintf(stdout, format, args);
    va_end(args);
}

void mxflib::error(const char *format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stderr, "MXFLib ERROR: ");
    vfprintf(stderr, format, args);
    va_end(args);
}


// mxf2dot messages

void mxf2dotDebug(const char* format, ...)
{
    if (!mxf2dotDebugFlag)
    {
        return;
    }
    
    va_list args;

    va_start(args, format);
    fprintf(stdout, "Debug: ");
    vprintf(format, args);
    va_end(args);
}

void mxf2dotWarning(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stdout, "Warning: ");
    vprintf(format, args);
    va_end(args);
}

void mxf2dotError(const char* format, ...)
{
    va_list args;

    va_start(args, format);
    fprintf(stderr, "ERROR: ");
    vfprintf(stderr, format, args);
    va_end(args);
}



// context data used when traversing the metadata object tree
class OutputContext
{
public:
    bool doSimple()
    {
        return oa != 0;
    }
    bool inArrayOrBatch()
    {
        return arrayOrBatchIndex >= 0;
    }
    void setInArrayOrBatch(bool yes)
    {
        if (yes)
        {
            arrayOrBatchIndex = 1;
        }
        else
        {
            arrayOrBatchIndex = -1;
        }
    }

    string getArrayOrBatchIndexStr()
    {
        char str[6];
        sprintf(str, "%d", arrayOrBatchIndex);
        return str;
    }

    
    int arrayOrBatchIndex;
    map<string, string>* oidToNodeId;
    MDObjectPtr obj;
    MDObjectPtr prop;
    DotObjectAttribute* oa;
    vector<pair<string, string> >* sourceRefs;
    string parentPackageUID;
};


// returns the id string used to identify a Track which is part of a Package
string getSourceRefTarget(string packageId, string trackId)
{
    return packageId + " " + trackId;
}

// returns true of the thing identified by the oid has a node id
bool haveNodeId(string oid, map<string, string>* oidToNodeId)
{
    assert(oid.length() > 0);
    map<string, string>::iterator item = oidToNodeId->find(oid);
    return item != oidToNodeId->end();
}

// returns the node id associated with the thing identified by the oid
string getNodeId(string oid, map<string, string>* oidToNodeId)
{
    if (oid.length() == 0)
    {
        mxf2dotError("Invalid object identification - object is probably missing a InstanceUID property\n");
        exit(1);
    }
    assert(oid.length() > 0);
    map<string, string>::iterator item = oidToNodeId->find(oid);
    assert(item != oidToNodeId->end());
    return (*item).second;
}



void outputProperty(DotFile* dotFile, OutputContext* context);

// output the object and object properties to dot
void outputObject(DotFile* dotFile, OutputContext* context)
{
    if (context->obj->IsA(GenericPackage_UL))
    {
        dotFile->startCluster(dotFile->getNextClusterId());
    }

    dotFile->startNode(getNodeId(context->obj->GetString(InstanceUID_UL), context->oidToNodeId));
    DotObjectAttribute oa;
    oa.setMaxPropertyWidth(60);
    oa.setId("label");
    oa.setObjectName(context->obj->Name());

    // simple properties
    MDObjectULList::iterator propIter;
    for (propIter = context->obj->begin(); propIter != context->obj->end(); propIter++)
    {
        context->setInArrayOrBatch(false);
        context->prop = (*propIter).second;
        context->oa = &oa;
        outputProperty(dotFile, context);
    }
    dotFile->writeAttribute(&oa);
    dotFile->writeAttribute("width", oa.getDisplayWidth(FONT_FIXED_PITCH));
    dotFile->endNode();
    // reference properties
    for (propIter = context->obj->begin(); propIter != context->obj->end(); propIter++)
    {
        context->setInArrayOrBatch(false);
        context->prop = (*propIter).second;
        context->oa = 0;
        outputProperty(dotFile, context);
    }
    
    if (context->obj->IsA(GenericPackage_UL))
    {
        dotFile->endCluster();
    }
}

// output the object property to dot
void outputProperty(DotFile* dotFile, OutputContext* context)
{
    if (context->prop->GetLink())
    {
        if (!context->doSimple())
        {
            string thisInstanceUID = context->obj->GetString(InstanceUID_UL); 
            string targetInstanceUID = context->prop->GetLink()->GetString(InstanceUID_UL);
            if (context->prop->GetRefType() == DICT_REF_STRONG)
            {
                // Strong reference value
                string sourceId = getNodeId(thisInstanceUID, context->oidToNodeId);
                string targetId = getNodeId(targetInstanceUID, context->oidToNodeId);
                dotFile->startEdge(sourceId, targetId);
                dotFile->writeAttribute("weight", "5.0");
                if (context->inArrayOrBatch())
                {
                    dotFile->writeAttribute("label", context->getArrayOrBatchIndexStr());
                }
                dotFile->endEdge();
                OutputContext newContext = *context;
                newContext.obj = context->prop->GetLink();
                outputObject(dotFile, &newContext);
            }
            else
            {
                // Weak reference value
                string sourceId = getNodeId(thisInstanceUID, context->oidToNodeId);
                string targetId = getNodeId(targetInstanceUID, context->oidToNodeId);
                dotFile->startEdge(sourceId, targetId);
                dotFile->writeAttribute("color", "blue");
                dotFile->writeAttribute("weight", "0.5");
                if (context->inArrayOrBatch())
                {
                    dotFile->writeAttribute("label", context->getArrayOrBatchIndexStr());
                }
                dotFile->endEdge();
            }
        }
    }
    else
    {
        if (context->prop->IsDValue())
        {
            // Value with unknown type
            if (context->doSimple())
            {
                context->oa->addProperty(context->prop->Name(), context->prop->GetString());
            }
        }
        else
        {
            if (context->prop->Value)
            {
                // Simple value or value with unknown type
                if (context->doSimple())
                {
                    context->oa->addProperty(context->prop->Name(), context->prop->GetString());
                }
                else
                {
                    // record source references (e.g. a SourceClip object)
                    // (pity there isn't an abstract SourceReference class!)
                    if (context->prop->IsA(SourcePackageID_UL) && 
                        context->obj->Child(SourceTrackID_UL))
                    {
                        string oid = getSourceRefTarget(context->prop->GetString(),
                            context->obj->Child(SourceTrackID_UL)->GetString());
                        if (haveNodeId(oid, context->oidToNodeId))
                        {
                            string sourceId = getNodeId(context->obj->GetString(InstanceUID_UL), context->oidToNodeId);
                            string targetId = getNodeId(oid, context->oidToNodeId);
                            // source package references will be output at the end, outside the clusters
                            context->sourceRefs->push_back(pair<string, string>(sourceId, targetId));
                        }
                    }
                }
            }
            else
            {
                // Array or batch value
                context->setInArrayOrBatch(true);
                MDObjectPtr prop = context->prop;
                MDObjectULList::iterator itemIter;
                for (itemIter = prop->begin(); itemIter != prop->end(); itemIter++)
                {
                    context->prop = (*itemIter).second;
                    outputProperty(dotFile, context);
                    context->arrayOrBatchIndex++;
                }
            }
        }
    }
}



// Record mapping from object InstanceUID and source reference target PackageUID/TrackId combination 
// to dot node ids
// The Preface object in a Closed Complete Header or Complete Footer is our root metadata object  
void preprocessObjects(int partitionNum, DotFile* dotFile, MXFFilePtr mxfFile, 
    map<string, string>& oidToNodeId, MDObjectPtr& root)
{
    mxfFile->GetRIP();

    root = 0;
    
    map<string, string> trackNodeIds;
    
    bool foundPartition = false;
    int partitionCount = 0;
    RIP::iterator iter;
    for (iter = mxfFile->FileRIP.begin(); iter != mxfFile->FileRIP.end(); iter++)
    {
        mxfFile->Seek((*iter).second->ByteOffset);
        PartitionPtr partition = mxfFile->ReadPartition();
        if (partition)
        {
            if (partitionNum == partitionCount ||
                partitionNum < 0 && 
                    (partition->IsA(ClosedCompleteHeader_UL) || 
                        partition->IsA(CompleteFooter_UL)))
            {
                foundPartition = true;
                if (partitionNum == partitionCount && 
                    !(partition->IsA(ClosedCompleteHeader_UL) || partition->IsA(CompleteFooter_UL)))
                {
                    mxf2dotWarning("Partition %d is not a closed complete header "
                        "or complete footer partition\n", partitionNum);
                }
                
                if (partition->ReadMetadata())
                {
                    MDObjectList::iterator objIter;
                    for (objIter = partition->AllMetadata.begin(); objIter != partition->AllMetadata.end(); objIter++)
                    {
                        // the Preface is our root
                        if ((*objIter)->IsA(Preface_UL))
                        {
                            root = *objIter;
                        }
                        
                        // record InstanceUID to Dot node ID mapping for every metadata object
                        string uid = (*objIter)->GetString(InstanceUID_UL);
                        if (uid.length() > 0)
                        {
                            string nodeId;
                            if ((*objIter)->IsA(Track_UL))
                            {
                                map<string, string>::const_iterator trackNodeIdIter = trackNodeIds.find(uid);
                                // use existing track node id
                                if (trackNodeIdIter != trackNodeIds.end())
                                {
                                    nodeId = (*trackNodeIdIter).second;
                                }
                                // else create a new one
                                else
                                {
                                    nodeId = dotFile->getNextNodeId();
                                    trackNodeIds.insert(pair<string, string>(uid, nodeId));
                                }
                            }
                            else
                            {
                                nodeId = dotFile->getNextNodeId();
                            }
                            oidToNodeId.insert(pair<string, string>(uid, nodeId));
                            
                            // record PackageUID/TrackId combination to a Track node id by traversing
                            // from the Package (to get the PackageUID) to the child Tracks
                            if ((*objIter)->IsA(GenericPackage_UL))
                            {
                                string packageId = (*objIter)->GetString(PackageUID_UL);
                                MDObjectPtr tracks = (*objIter)->Child(Tracks_UL);
                                if (tracks)
                                {
                                    MDObjectULList::iterator trackIter;
                                    for (trackIter = tracks->begin(); trackIter != tracks->end(); trackIter++)
                                    {
                                        MDObjectPtr track = (*trackIter).second;
                                        if (track->GetLink())
                                        {
                                            string trackUID = track->GetLink()->GetString(InstanceUID_UL);
                                            string trackId = track->GetLink()->GetString(TrackID_UL);
                                            string trackNodeId;
                                            map<string, string>::const_iterator trackNodeIdIter = trackNodeIds.find(trackUID);
                                            // use existing track node id
                                            if (trackNodeIdIter != trackNodeIds.end())
                                            {
                                                trackNodeId = (*trackNodeIdIter).second;
                                            }
                                            // else create a new one 
                                            else
                                            {
                                                trackNodeId = dotFile->getNextNodeId();
                                                trackNodeIds.insert(pair<string, string>(trackUID, trackNodeId));
                                            }
                                            oidToNodeId.insert(pair<string, string>(
                                                getSourceRefTarget(packageId, trackId), trackNodeId));
                                        }
                                    }
                                }
                            }
                        }
                    }
                    break;
                }
                else
                {
                    mxf2dotError("Failed to read partition metadata\n");
                    break;
                }
            }
        }
        partitionCount++;
    }
    
    if (partitionNum >= 0 && !foundPartition)
    {
        mxf2dotError("Failed to find partition %d in a total of %d partitions\n", partitionNum, partitionCount);
    }
    else if (partitionNum < 0 && !foundPartition)
    {
        mxf2dotError("Failed to find closed complete header or complete footer partition "
            "in total of %d partitions\n", partitionCount);
    }
    
}



int convert(int partitionNum, MXFFilePtr mxfFile, DotFile* dotFile)
{
    map<string, string> oidToNodeId;
    MDObjectPtr root;
    // the first pass records info which is used when traversing and outputting 
    // the object tree later
    preprocessObjects(partitionNum, dotFile, mxfFile, oidToNodeId, root);
    if (!root)
    {
        mxf2dotError("No valid Preface object found\n");
        return 1;
    }
    
    // start output with default attributes
    dotFile->startGraph("MXF");
    dotFile->startDefaultAttributes("graph");
    dotFile->writeAttribute("concentrate", "false");
    dotFile->endDefaultAttributes();
    dotFile->startDefaultAttributes("node");
    dotFile->writeAttribute("fontname", FONT_NAME);
    dotFile->writeAttribute("fontsize", FONT_SIZE);
    dotFile->writeAttribute("shape", "record");
    dotFile->endDefaultAttributes();
    dotFile->startDefaultAttributes("edge");
    dotFile->writeAttribute("color", "black");
    dotFile->writeAttribute("style", "solid");
    dotFile->writeAttribute("weight", "1");
    dotFile->endDefaultAttributes();
    
    // output by traversing the object tree
    vector<pair<string, string> > sourceRefs;
    OutputContext context;
    context.obj = root;
    context.sourceRefs = &sourceRefs;
    context.oidToNodeId = &oidToNodeId;
    outputObject(dotFile, &context);

    // output source package references which must be output outside clusters
    vector<pair<string, string> >::const_iterator iter;
    for (iter = sourceRefs.begin(); iter != sourceRefs.end(); iter++)
    {
        dotFile->startEdge((*iter).first, (*iter).second);
        dotFile->writeAttribute("color", "orange");
        dotFile->writeAttribute("weight", "10.0");
        dotFile->endEdge();
    }

    dotFile->endGraph();
    
    return 0;
}



void printUsage(const char* cmd)
{
    fprintf(stderr, "Usage: %s [options] <mxf input file> <dot output file>\n", cmd); 
    fprintf(stderr, "Options:\n"); 
    fprintf(stderr, "  -d <dict>     Load supplementary dictionary.\n");
#ifdef COMPILED_DICT
    fprintf(stderr, "  -m <dict>     Specify main dictionary (instead of compile-time version)\n");
#else
    fprintf(stderr, "  -m <dict>     Specify main dictionary (instead of dict.xml)\n");
#endif
    fprintf(stderr, "  -p <n>        Dump metadata in partition <n> (n >= 0).\n");
    fprintf(stderr, "  -v            Show debug messages\n\n"); 
}

int main(int argc, const char** argv)
{
    vector<string> suppDicts;
    string dictName;
    string inputFilename;
    string outputFilename;
    int partitionNum = -1;
    int i = 1;
     while (i < argc)
    {
        if (!strcmp(argv[i], "-v"))
        {
            mxf2dotDebugFlag = true;
            mxfLibDebugFlag = true;
            i++;
        }
        else if (!strcmp(argv[i], "-d"))
        {
            i++;
            if (i >= argc)
            {
                printUsage(argv[0]);
                fprintf(stderr, "Missing argument for option -d\n");
                exit(1);
            }
            suppDicts.push_back(argv[i]);
            i++;
        }
        else if (!strcmp(argv[i], "-m"))
        {
            i++;
            if (i >= argc)
            {
                printUsage(argv[0]);
                fprintf(stderr, "Missing argument for option -m\n");
                exit(1);
            }
            dictName = argv[i];
            i++;
        }
        else if (!strcmp(argv[i], "-p"))
        {
            i++;
            if (i >= argc)
            {
                printUsage(argv[0]);
                fprintf(stderr, "Missing argument for option -p\n");
                exit(1);
            }
            int result = sscanf(argv[i], "%d", &partitionNum);
            if (result != 1)
            {
                printUsage(argv[0]);
                fprintf(stderr, "Failed to read -p argument\n");
                exit(1);
            }
            i++;
        }
        else if (inputFilename.length() == 0)
        {
            inputFilename = argv[i];
            i++;
        }
        else if (outputFilename.length() == 0)
        {
            outputFilename = argv[i];
            i++;
        }
        else
        {
            printUsage(argv[0]);
            fprintf(stderr, "Unexpected argument: %s\n", argv[i]);
            exit(1);
        }
    }
    
    if (inputFilename.length() == 0)
    {
        printUsage(argv[0]);
        fprintf(stderr, "Missing input and output filenames\n");
        exit(1);
    }
    else if (outputFilename.length() == 0)
    {
        printUsage(argv[0]);
        fprintf(stderr, "Missing output filename\n");
        exit(1);
    }


    // load main dictionary
    if (dictName.length() == 0)
    {
#ifdef COMPILED_DICT
        LoadDictionary(DictData);
#else
        LoadDictionary("dict.xml");
#endif
    }    
    else
    {
        LoadDictionary(dictName);
    }
    
    // load supplementary dictionaries
    vector<string>::const_iterator iter;
    for (iter = suppDicts.begin(); iter != suppDicts.end(); iter++)
    {
        LoadDictionary(*iter);
    }


    // open dot output file
    DotFile dotFile(outputFilename.c_str());

    // open mxf input file
    MXFFilePtr mxfFile = new MXFFile();
    if (!mxfFile->Open(inputFilename.c_str(), true))
    {
        perror(inputFilename.c_str());
        exit(1);
    }

    // mxf 2 dot
    int result = convert(partitionNum, mxfFile, &dotFile);

    mxfFile->Close();
    
    return result;
}




