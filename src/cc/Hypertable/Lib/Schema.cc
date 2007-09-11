/**
 * Copyright (C) 2007 Doug Judd (Zvents, Inc.)
 * 
 * This file is part of Hypertable.
 * 
 * Hypertable is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or any later version.
 * 
 * Hypertable is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */


#include <iostream>
#include <string>

#include <boost/algorithm/string.hpp>

#include <expat.h>

extern "C" {
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
}

#include "Common/FileUtils.h"
#include "Common/Logger.h"
#include "Common/StringExt.h"
#include "Common/System.h"

#include "Schema.h"

using namespace hypertable;
using namespace hypertable;
using namespace std;

Schema *Schema::msSchema = 0;
std::string Schema::msCollectedText = "";
boost::mutex Schema::msMutex;


/**
 */
Schema::Schema(bool readIds) : mErrorString(), mNextColumnId(0), mAccessGroupMap(), mColumnFamilyMap(), mGeneration(1), mAccessGroups(), mOpenAccessGroup(0), mOpenColumnFamily(0), mReadIds(readIds), mOutputIds(false), mMaxColumnFamilyId(0) {
  if (Logger::logger == 0) {
    cerr << "Logger::Initialize must be called before using Schema class" << endl;
    exit(1);
  }
}


Schema *Schema::NewInstance(const char *buf, int len, bool readIds) {
  boost::mutex::scoped_lock lock(msMutex);
  XML_Parser parser = XML_ParserCreate("US-ASCII");

  XML_SetElementHandler(parser, &StartElementHandler, &EndElementHandler);
  XML_SetCharacterDataHandler(parser, &CharacterDataHandler);

  msSchema = new Schema(readIds);

  if (XML_Parse(parser, buf, len, 1) == 0) {
    std::string errStr = (std::string)"Schema Parse Error: " + (const char *)XML_ErrorString(XML_GetErrorCode(parser)) + " line " + (int)XML_GetCurrentLineNumber(parser) + ", offset " + (int)XML_GetCurrentByteIndex(parser);
    msSchema->SetErrorString(errStr);
  }

  XML_ParserFree(parser);  

  Schema *schema = msSchema;
  msSchema = 0;
  return schema;
}


/**
 */
void Schema::StartElementHandler(void *userData,
			 const XML_Char *name,
			 const XML_Char **atts) {
  int i;

  if (!strcmp(name, "Schema")) {
    for (i=0; atts[i] != 0; i+=2) {
      if (atts[i+1] == 0)
	return;
      if (msSchema->mReadIds && !strcmp(atts[i], "generation"))
	msSchema->SetGeneration(atts[i+1]);
      else
	msSchema->SetErrorString((string)"Unrecognized 'Schema' attribute : " + atts[i]);
    }
  }
  else if (!strcmp(name, "AccessGroup")) {
    msSchema->OpenAccessGroup();    
    for (i=0; atts[i] != 0; i+=2) {
      if (atts[i+1] == 0)
	return;
      msSchema->SetAccessGroupParameter(atts[i], atts[i+1]);
    }
  }
  else if (!strcmp(name, "ColumnFamily")) {
    msSchema->OpenColumnFamily();
    for (i=0; atts[i] != 0; i+=2) {
      if (atts[i+1] == 0)
	return;
      msSchema->SetColumnFamilyParameter(atts[i], atts[i+1]);
    }
  }
  else if (!strcmp(name, "CellLimit") || !strcmp(name, "ExpireDays") || !strcmp(name, "Name"))
    msCollectedText = "";
  else
    msSchema->SetErrorString((string)"Unrecognized element - '" + name + "'");

  return;
}


void Schema::EndElementHandler(void *userData, const XML_Char *name) {
  if (!strcmp(name, "AccessGroup"))
    msSchema->CloseAccessGroup();
  else if (!strcmp(name, "ColumnFamily"))
    msSchema->CloseColumnFamily();
  else if (!strcmp(name, "CellLimit") || !strcmp(name, "ExpireDays") || !strcmp(name, "Name")) {
    boost::trim(msCollectedText);
    msSchema->SetColumnFamilyParameter(name, msCollectedText.c_str());
  }
}



void Schema::CharacterDataHandler(void *userData, const XML_Char *s, int len) {
  msCollectedText.assign(s, len);
}



void Schema::OpenAccessGroup() {
  if (mOpenAccessGroup != 0)
    SetErrorString((string)"Nested AccessGroup elements not allowed");
  else {
    mOpenAccessGroup = new AccessGroup();
  }
}



void Schema::CloseAccessGroup() {
  if (mOpenAccessGroup == 0)
    SetErrorString((string)"Unable to close locality group, not open");
  else {
    if (mOpenAccessGroup->name == "")
      SetErrorString((string)"Name attribute required for all AccessGroup elements");
    else {
      if (mAccessGroupMap.find(mOpenAccessGroup->name) != mAccessGroupMap.end())
	SetErrorString((string)"Multiply defined locality group  '" + mOpenAccessGroup->name + "'");
      else {
	mAccessGroupMap[mOpenAccessGroup->name] = mOpenAccessGroup;
	mAccessGroups.push_back(mOpenAccessGroup);
      }
    }
    mOpenAccessGroup = 0;
  }
}



void Schema::OpenColumnFamily() {
  if (mOpenAccessGroup == 0)
    SetErrorString((string)"ColumnFamily defined outside of AccessGroup");
  else {
    if (mOpenColumnFamily != 0)
      SetErrorString((string)"Nested ColumnFamily elements not allowed");
    else {
      mOpenColumnFamily = new ColumnFamily();
      mOpenColumnFamily->id = 0;
      mOpenColumnFamily->cellLimit = 0;
      mOpenColumnFamily->expireTime = 0;
      mOpenColumnFamily->lg = mOpenAccessGroup->name;
    }
  }
}



void Schema::CloseColumnFamily() {
  if (mOpenAccessGroup == 0)
    SetErrorString((string)"ColumnFamily defined outside of AccessGroup");
  else if (mOpenColumnFamily == 0)
    SetErrorString((string)"Unable to close column family, not open");
  else {
    if (mOpenColumnFamily->name == "")
      SetErrorString((string)"ColumnFamily must have Name child element");
    else if (mReadIds && mOpenColumnFamily->id == 0) {
      SetErrorString((string)"No id specifid for ColumnFamily '" + mOpenColumnFamily->name + "'");
    }
    else {
      if (mColumnFamilyMap.find(mOpenColumnFamily->name) != mColumnFamilyMap.end())
	SetErrorString((string)"Multiply defined column families '" + mOpenColumnFamily->name + "'");
      else {
	mColumnFamilyMap[mOpenColumnFamily->name] = mOpenColumnFamily;
	if (mReadIds)
	  mColumnFamilyIdMap[mOpenColumnFamily->id] = mOpenColumnFamily;
	mOpenAccessGroup->columns.push_back(mOpenColumnFamily);
      }
      mOpenColumnFamily = 0;
    }
  }
}



void Schema::SetAccessGroupParameter(const char *param, const char *value) {
  if (mOpenAccessGroup == 0)
    SetErrorString((string)"Unable to set AccessGroup attribute '" + param + "', locality group not open");
  else {
    if (!strcasecmp(param, "name"))
      mOpenAccessGroup->name = value;
    else if (!strcasecmp(param, "inMemory")) {
      if (!strcasecmp(value, "true") || !strcmp(value, "1"))
	mOpenAccessGroup->inMemory = true;
      else if (!strcasecmp(value, "false") || !strcmp(value, "0"))
	mOpenAccessGroup->inMemory = false;
      else
	SetErrorString((string)"Invalid value (" + value + ") for AccessGroup attribute '" + param + "'");
    }
    else
      SetErrorString((string)"Invalid AccessGroup attribute '" + param + "'");
  }
}



void Schema::SetColumnFamilyParameter(const char *param, const char *value) {
  if (mOpenColumnFamily == 0)
    SetErrorString((string)"Unable to set ColumnFamily parameter '" + param + "', column family not open");
  else {
    if (!strcasecmp(param, "Name"))
      mOpenColumnFamily->name = value;
    else if (!strcasecmp(param, "ExpireDays")) {
      float fval = strtof(value, 0);
      if (fval == 0.0) {
	SetErrorString((string)"Invalid value (" + value + ") for ExpireDays");
      }
      else {
	fval *= 86400.0;
	mOpenColumnFamily->expireTime = (time_t)fval;
      }
    }
    else if (!strcasecmp(param, "CellLimit")) {
      mOpenColumnFamily->cellLimit = atoi(value);
      if (mOpenColumnFamily->cellLimit == 0)
	SetErrorString((string)"Invalid value (" + value + ") for CellLimit");
    }
    else if (mReadIds && !strcasecmp(param, "id")) {
      mOpenColumnFamily->id = atoi(value);
      if (mOpenColumnFamily->id == 0)
	SetErrorString((string)"Invalid value (" + value + ") for ColumnFamily id attribute");
      if (mOpenColumnFamily->id > mMaxColumnFamilyId)
	mMaxColumnFamilyId = mOpenColumnFamily->id;
    }
    else
      SetErrorString((string)"Invalid ColumnFamily parameter '" + param + "'");
  }
}


void Schema::AssignIds() {
  mMaxColumnFamilyId = 0;
  for (list<AccessGroup *>::iterator lgIter = mAccessGroups.begin(); lgIter != mAccessGroups.end(); lgIter++) {
    for (list<ColumnFamily *>::iterator cfIter = (*lgIter)->columns.begin(); cfIter != (*lgIter)->columns.end(); cfIter++) {
      (*cfIter)->id = ++mMaxColumnFamilyId;
    }
  }
  mGeneration = 1;
  mOutputIds=true;
}


void Schema::Render(std::string &output) {
  char buf[64];

  if (!IsValid()) {
    output = mErrorString;
    return;
  }

  output += "<Schema";
  if (mOutputIds) {
    output += " generation=\"";
    sprintf(buf, "%d", mGeneration);
    output += (string)buf + "\"";
  }
  output += ">\n";

  for (list<AccessGroup *>::iterator iter = mAccessGroups.begin(); iter != mAccessGroups.end(); iter++) {
    output += (string)"  <AccessGroup name=\"" + (*iter)->name + "\"";
    if ((*iter)->inMemory)
      output += " inMemory=\"true\"";
    output += ">\n";
    for (list<ColumnFamily *>::iterator cfiter = (*iter)->columns.begin(); cfiter != (*iter)->columns.end(); cfiter++) {
      output += (string)"    <ColumnFamily";
      if (mOutputIds) {
	output += " id=\"";
	sprintf(buf, "%d", (*cfiter)->id);
	output += (string)buf + "\"";
      }
      output += ">\n";
      output += (string)"      <Name>" + (*cfiter)->name + "</Name>\n";
      if ((*cfiter)->cellLimit != 0) {
	sprintf(buf, "%d", (*cfiter)->cellLimit);
	output += (string)"        <CellLimit>" + buf + "</CellLimit>\n";
      }
      if ((*cfiter)->expireTime != 0) {
	float fdays = (float)(*cfiter)->expireTime / 86400.0;
	sprintf(buf, "%.2f", fdays);
	output += (string)"        <ExpireDays>" + buf + "</ExpireDays>\n";
      }
      output += (string)"    </ColumnFamily>\n";
    }
    output += (string)"  </AccessGroup>\n";
  }

  output += "</Schema>\n";
  return;
}


/**
 * Protected methods
 */

bool Schema::IsValid() {
  if (GetErrorString() != 0)
    return false;
  return true;
}

/**
 *
 */
void Schema::SetGeneration(const char *generation) {
  int genNum = atoi(generation);
  if (genNum <= 0)
    SetErrorString((string)"Invalid Table 'generation' value (" + generation + ")");
  mGeneration = genNum;
}

