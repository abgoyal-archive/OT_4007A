#! /usr/bin/python

import xml.dom.minidom as xdom
import shutil
import sys
import os
import re
import zipfile

class CustXmlExt(object):
    def __init__(self,xml):
	dom = CustXmlParser(xml)
        self.srcrel = []
        self.binrel = []
        self.remrel = []
	self.DirList = []
        self.FileList = []
        self.AndroidSourceList = []
        self.AndroidBinaryList = []
        self.FrameworkSourceList = []
        self.FrameworkBinaryList = []
        self.FrameworkPartialList = []
        self.AppSourceList = []
        self.AppBinaryList = []
        self.unrelDirList = []
        self.unrelFileList = []
        self.KernelBinaryList = []
        self.KernelSourceList = []
        try:
        	self.DirList = dom.getDirList()
		self.FileList = dom.getFileList()
		self.AndroidSourceList = dom.getAndroidSourceList()
		self.AndroidBinaryList = dom.getAndroidBinaryList()
		self.FrameworkSourceList = dom.getFrameworkSourceList()
		self.FrameworkBinaryList = dom.getFrameworkBinaryList()
		self.FrameworkPartialList = dom.getFrameworkPartialList()
		self.AppSourceList = dom.getAppSourceList()
		self.AppBinaryList = dom.getAppBinaryList()
		self.unrelDirList = dom.getUnreleaseDirList()
        	self.unrelFileList = dom.getUnreleaseFileList()
        	self.KernelBinaryList = dom.getKernelBinaryList()
        	self.KernelSourceList = dom.getKernelSourceList()
	except Exception, e:
		print "[ERROR: CustXmlExt.__init__] %s" % str(e)

    def GetAllSourceList(self):
        srclist = []
        srclist.extend(self.AndroidSourceList)
	srclist.extend(self.FrameworkSourceList)
	srclist.extend(self.AppSourceList)
	srclist.extend(self.DirList)
	srclist.extend(self.FileList)
	srclist.extend(self.KernelSourceList)
        return srclist

    def GetAllBinaryList(self):
        binlist = []
	binlist.extend(self.AndroidBinaryList)
	binlist.extend(self.AppBinaryList)
	binlist.extend(self.KernelBinaryList)
        for pl in self.FrameworkPartialList:
        	for plItem in self.FrameworkPartialList[pl]:
            		base = plItem["base"]
            		binary_list = plItem["binary_list"]
            		for bl in binary_list:
                		blfull = os.path.join(base,bl)
                        	self.binrel.append(blfull)
        return binlist

    def GetAllFrameworksBinaryList(self):
	binlist = {}
	for pl in self.FrameworkPartialList:
                for plItem in self.FrameworkPartialList[pl]:
                        base = plItem["base"]
                        binary_list = plItem["binary_list"]
                        for bl in binary_list:
                                blfull = os.path.join(base,bl)
                                binlist[blfull] = bl
	return binlist

    def GetAllRemoveList(self):
	remlist = []
        remlist.extend(self.unrelDirList)
        remlist.extend(self.unrelFileList)
        return remlist

    def GenReleasePolicyHash(self):
	policyhash = {}
	srclist = self.GetAllSourceList()
	binlist = self.GetAllBinaryList()
	rmlist = self.GetAllRemoveList()
	fmbinlist = self.GetAllFrameworksBinaryList()
	for src in srclist:
        	policyhash[src] = 'source'
        for bi in binlist:
        	policyhash[bi] = 'binary'
        for rm in rmlist:
        	policyhash[rm] = 'remove'
	return policyhash

    def GetReleasePolicy(self,item):
        if item != '':
		policyhash = self.GenReleasePolicyHash()
        	if policyhash.has_key(item):
                	return policyhash[item]
        	else:
                	return 'none'
	else:
		print >> sys.stderr,"[ERROR: GetReleasePolicy] %s can not be empty" % item

    def RemoveReleasePolicy(self,item):
        pass

    def AddReleasePolicy(self,item,policy,catigory):
        pass


# define our own  XML-DOM class for custom release policy
class CustXmlParser(object):
    def __init__(self,xml):
        self.xmlDom = xdom.parse(xml)

    def getRoot(self):
        return self.xmlDom.documentElement

    def getDirList(self):
        root = self.getRoot()
        dirElement = root.getElementsByTagName("DirList")[0].getElementsByTagName("ReleaseDirList")[0].getElementsByTagName("Dir")
        dirList = map(str,[item.firstChild.nodeValue for item in dirElement if item.firstChild is not None])
        return dirList

    def getUnreleaseDirList(self):
        root = self.getRoot()
        dirElement = root.getElementsByTagName("DirList")[0].getElementsByTagName("UnReleaseDirList")[0].getElementsByTagName("Dir")
        dirList = map(str,[item.firstChild.nodeValue for item in dirElement if item.firstChild is not None])
        return dirList

    def getFileList(self):
        root = self.getRoot()
        fileElement = root.getElementsByTagName("FileList")[0].getElementsByTagName("ReleaseFileList")[0].getElementsByTagName("File")
        fileList = map(str,[item.firstChild.nodeValue for item in fileElement if item.firstChild is not None])
        return fileList

    def getUnreleaseFileList(self):
        root = self.getRoot()
        fileElement = root.getElementsByTagName("FileList")[0].getElementsByTagName("UnReleaseFileList")[0].getElementsByTagName("File")
        fileList = map(str,[item.firstChild.nodeValue for item in fileElement if item.firstChild is not None])
        return fileList

    def getKernelSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("KernelRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getKernelBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("KernelRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getAppSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("APPRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getAppBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("APPRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getAndroidSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("AndroidRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getAndroidBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("AndroidRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getFrameworkSourceList(self):
        root = self.getRoot()
        sourceElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("SourceList")[0].getElementsByTagName("Source")
        sourceList = map(str,[item.firstChild.nodeValue for item in sourceElement if item.firstChild is not None])
        return sourceList

    def getFrameworkBinaryList(self):
        root = self.getRoot()
        binaryElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("BINList")[0].getElementsByTagName("Binary")
        binaryList = map(str,[item.firstChild.nodeValue for item in binaryElement if item.firstChild is not None])
        return binaryList

    def getFrameworkPartialList(self):
        root = self.getRoot()
        partialElement = root.getElementsByTagName("FrameworkRelease")[0].getElementsByTagName("PartialSourceList")[0].getElementsByTagName("PartialSource")
        frameworkDict = {}
        for x in partialElement:
            module = str(x.getAttribute("module"))
            base = str(x.getAttribute("base"))
            binaryList = map(str,[item.firstChild.nodeValue for item in x.getElementsByTagName("Binary") if item.firstChild is not None])
            d = {}
            d["base"] = base
            d["binary_list"] = binaryList
            frameworkDict.setdefault(module,[]).append(d) 
        return frameworkDict

#end XmlDom

xml = "rel_customer_basic.xml"
a = CustXmlExt(xml)
b = a.GenReleasePolicyHash()
print "[resutl] %s" % a.GetReleasePolicy("bionic")
for key in b.keys():
	print "%s=>%s" % (b[key],key)

