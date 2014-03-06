'''
      -*- OpenSAF  -*-

 (C) Copyright 2009 The OpenSAF Foundation

 This program is distributed in the hope that it will be useful, but
 WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE. This file and program are licensed
 under the GNU Lesser General Public License Version 2.1, February 1999.
 The complete license can be accessed from the following location:
 http://opensource.org/licenses/lgpl-license.php
 See the Copying file included with the OpenSAF distribution for full
 licensing terms.

 Author(s): Ericsson AB
'''

import sys,glob,os
from subprocess import call

class BaseOptions:
    traceOn = False

    @staticmethod
    def printOptionSettings():
        return 'Options:\n traceOn:%s\n' % (BaseOptions.traceOn)



class BaseImmDocument:
    imm_content_element_name = "imm:IMM-contents"
    imm_content_no_namespace_spec = "<imm:IMM-contents>"
    imm_content_with_namespace_spec = "<imm:IMM-contents xmlns:imm=\"http://www.saforum.org/IMMSchema\" xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"SAI-AIS-IMM-XSD-A.02.13.xsd\">"
    
     # helper function to identify a problem where python/minidom
    # fails to parse some imm.xml files where the imm: namespace is missing
    # (for example the output of the immdump tool)
    def verifyInputXmlDocumentFileIsParsable(self, filename):
        if os.stat(filename).st_size == 0:
            abort_script("The inputfile %s is empty!", filename)
            
        f = open(filename)
        immContentsTagFound = False
        for line in f:
            s = line
            str = s.replace(self.imm_content_no_namespace_spec, self.imm_content_with_namespace_spec)
            if len(str) != len(s):
                # there was a substitution....file will not be possible to parse....
                abort_script("The inputfile lacks namespace specification in <imm:IMM-contents> tag")
            elif s.find(self.imm_content_element_name) != -1:
                # Assume file is ok w.r.t namespace 
                return
            
            

    def validateXmlFileWithSchema(self, filename, schemaFilename):
        trace("")
                
        trace("validate xml file:%s", filename)
        # validate the result file with xmllint
        command = "/usr/bin/xmllint --noout"+" --schema "+schemaFilename+" "+filename
        print_info_stderr("XML Schema validation (using xmllint):")
        trace("validate command:%s", command)
        retCode = call(command, shell=True)
        if retCode != 0:
            return retCode

        
        trace("Successfully validated xml document file: %s", filename)
        return 0

    def formatXmlFileWithXmlLint(self, infilename, outfilename):
        trace("formatXmlFileWithXmlLint() prettify xml file:%s", infilename)
        # "prettify" the result file with xmllint (due to inadequate python/minidom prettify functionality)
        #command = "/bin/sh -c 'XMLLINT_INDENT=\"	\" ; export XMLLINT_INDENT; /usr/bin/xmllint --format "+infilename+" --output "+outfilename +"'"
        command = "/bin/sh -c 'XMLLINT_INDENT=\""+"\t"+"\" ; export XMLLINT_INDENT; /usr/bin/xmllint --format "+infilename+" --output "+outfilename +"'"
        trace("formatting command:%s", command)
        retCode = call(command, shell=True)
        #if retCode != 0:
            #return retCode
            #validate_failed("failed to validate input file %s:", filename)
        return retCode

    def printToStdoutAndRemoveTmpFile(self, tempfile):
        # Print the tmp file to the standard output
        trace("Print the tmp file to the standard output: %s", tempfile)
        f = file(tempfile)
        while True:
            line = f.readline()
            if len(line) == 0:
                break
            print line, # notice comma
        f.close()
        # remove temp file
        trace("delete the stdout tmp file: %s", tempfile)
        os.remove(tempfile)

    # helper function to handle a problem where python/minidom
    # fails to parse some imm.xml files where the imm: namespace is missing
    # (for example the output of the immdump tool)
    # Currently NOT used.....
    def openXmlDocumentFileAndCheckNamespace(self, filename):
        f = open(filename)
        str_list = []
        immContentsTagFound = False
        immContentsTagReplaced = False
        for line in f:
            s = line
            if (immContentsTagFound == False):
                str = s.replace(self.imm_content_no_namespace_spec, self.imm_content_with_namespace_spec)
                if len(str) != len(s):
                    s = str
                    immContentsTagFound = True
                    immContentsTagReplaced = True
                elif s.find(self.imm_content_element_name) != -1:
                    immContentsTagFound = True    
            
            str_list.append(s) 
        
        xmlstr = ' '.join(str_list)
        
        
        if Options.schemaFilename != None:
            if immContentsTagReplaced == True:
                print_info_stderr("Cannot validate inputfile '%s' with schema file because of missing namespace specification in element <imm:IMM-contents>. \nProceeding with processing of modified input data!", filename)
            else:
                if self.validateXmlFile(filename) != 0:
                    abort_script("Failed to validate the inputfile %s:", filename)

        
        return xml.dom.minidom.parseString(xmlstr)

   # helper function to remove some whitespace which we do not want to have in result of toPrettyXml
    # Currently NOT used.....
    def openXmlDocumentAndStrip(self, filename):
        f = open(filename)
        str_list = []
        for line in f:
            #s = line.rstrip().lstrip()
            s = line.strip('\t\n')
            if len(s) < 5:
                trace("short line *%s*", s)
            str_list.append(s) 
        
        xmlstr = ' '.join(str_list)
        return xml.dom.minidom.parseString(xmlstr)
# end of Class BaseImmDocument    


def trace(*args):
    if BaseOptions.traceOn == True:
        printf_args = []
        for i in range(1, len(args)):
            printf_args.append(args[i])
    
        formatStr = "TRACE:\t" + args[0]
        print >> sys.stderr, formatStr % tuple(printf_args)


def retrieveFilenames(args):
    fileList = []

    trace("before glob args:%s", args)
    fileList = glob.glob(args[0])  # wildcard expansion for WIN support, however not tested....
    trace("after glob filelist:%s length:%d", fileList, len(fileList))

    if (len(fileList) < 2 and len(args) >= 1):
        fileList = args        
    
    trace("Final filelist:%s length:%d", fileList, len(fileList))
    return fileList


def print_info_stderr(*args):
    printf_args = []
    for i in range(1, len(args)):
        printf_args.append(args[i])
    
    formatStr = args[0]
    print >> sys.stderr, formatStr % tuple(printf_args)
    

def abort_script(*args):
    printf_args = []
    for i in range(1, len(args)):
        printf_args.append(args[i])
    
    formatStr = "\nAborting script: " + args[0]
    print >> sys.stderr, formatStr % tuple(printf_args)
    sys.exit(2)

def exit_script(*args):
    printf_args = []
    for i in range(1, len(args)):
        printf_args.append(args[i])
    
    formatStr = "\n" + args[0]
    print >> sys.stderr, formatStr % tuple(printf_args)
    sys.exit(0)    

def verifyInputFileReadAcess(filename):
    if os.access(filename, os.R_OK) == False:
        abort_script("Cannot access input file: %s", filename)    


# An attempt to fix minidom pretty print functionality
# see http://ronrothman.com/public/leftbraned/xml-dom-minidom-toprettyxml-and-silly-whitespace/
# however the result is still not nice..... (have replaced it with xmllint formatting) 
# This way the method gets replaced, worked when called from main()
    # attempt to replace minidom's function with a "hacked" version
    # to get decent prettyXml, however it does not look nice anyway....
    #xml.dom.minidom.Element.writexml = fixed_writexml

def fixed_writexml(self, writer, indent="", addindent="", newl=""):
    # indent = current indentation
    # addindent = indentation to add to higher levels
    # newl = newline string
    writer.write(indent+"<" + self.tagName)

    attrs = self._get_attributes()
    a_names = attrs.keys()
    a_names.sort()

    for a_name in a_names:
        writer.write(" %s=\"" % a_name)
        xml.dom.minidom._write_data(writer, attrs[a_name].value)
        writer.write("\"")
    if self.childNodes:
        if len(self.childNodes) == 1 \
          and self.childNodes[0].nodeType == xml.dom.minidom.Node.TEXT_NODE:
            writer.write(">")
            self.childNodes[0].writexml(writer, "", "", "")
            writer.write("</%s>%s" % (self.tagName, newl))
            return
        writer.write(">%s"%(newl))
        for node in self.childNodes:
            node.writexml(writer,indent+addindent,addindent,newl)
        writer.write("%s</%s>%s" % (indent,self.tagName,newl))
    else:
        writer.write("/>%s"%(newl))

