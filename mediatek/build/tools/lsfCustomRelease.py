#! /usr/bin/python

import xml.dom.minidom as xdom
from optparse import OptionParser
import shutil
import sys
import os
import re
import zipfile
import time
import threading
import pexpect

#global
fileServerPath = '/release/public1/YuSu_release/mpreleaseTest'

#do not modify
lsfBuildPath = '/alps/mprelease_lsf'
googlePureSource_p4 = '/proj/mtk80469/mprelease/p4/default/ALPS_SW/TRUNK/android'
googlePureSource_lsf = os.path.join(lsfBuildPath,"google")

parser = OptionParser(usage="usage: %prog srcPath destPath project releasePolicyXml",version="%prog 1.0")
(options,args) = parser.parse_args()
if len(args) != 4:
    parser.print_help()
    sys.exit(1)

class Arguments(object):pass
ARGUMENTS = Arguments()
ARGUMENTS.srcPath = os.path.abspath(args[0])
ARGUMENTS.destPath = os.path.abspath(args[1])
ARGUMENTS.project = args[2]
#ARGUMENTS.xml = os.path.abspath(args[3])
ARGUMENTS.xml = args[3]

# check the arguments correctness
def checkArgument(argu):
    """ check the argument """
    if not os.path.exists(argu.srcPath):
        print >> sys.stderr,"the input srcPath '%s' does not exist!" % argu.srcPath
        sys.exit(1)
    projConfigFolder = "%s/mediatek/config/%s" % (argu.srcPath,argu.project)
    if not os.path.exists(projConfigFolder):
        print >> sys.stderr,"the input project '%s' is illegal,please check it!" % argu.project
        sys.exit(1)

checkArgument(ARGUMENTS)


class VerifyService(object):
    def __init__(self):
        """ variable initialization """
        self.srcPath = ARGUMENTS.srcPath
        self.destPath = ARGUMENTS.destPath
        self.project = ARGUMENTS.project
        self.policy = ARGUMENTS.xml
        self.lsfBuildPath = lsfBuildPath
        self.fullCodebase = os.path.join(self.lsfBuildPath,"source/alps/")
        self.verifyCodebase = os.path.join(self.lsfBuildPath,"verify/alps/")

    def clean(self):
        os.system("rm -rf %s" % self.lsfBuildPath)
        #os.system("chmod -R 775 %s/" % self.lsfBuildPath)

    def resetAndroid(self):
        pass

    def getCodebase(self):
        if not os.path.exists(self.fullCodebase):
            os.system("mkdir -p %s" % self.fullCodebase)
        print >> sys.stdout,"rsync -a --chmod=a+rwx --delete --force %s/ %s" % (self.srcPath,self.fullCodebase)
        os.system("rsync -a --chmod=a+rwx --delete --force %s/ %s" % (self.srcPath,self.fullCodebase))
        #self.userSync(self.srcPath,self.fullCodebase)

    def build(self,action,mode):
        print >> sys.stdout,"./makeMtk -o=%s %s %s" % (mode,self.project,action)
        os.system("./makeMtk -o=%s %s %s" % (mode,self.project,action))
        self.copyLog('./')
        if self.checkLog():
            print >> sys.stderr,"Error! %s build %s failed!" % (self.project,action)
            #sys.exit(2)
            os._exit(1)
        else:
            print >> sys.stdout,"Project %s build %s pass!" % (self.project,action)

    def fullBuild(self):
        if not os.path.exists(self.fullCodebase):
            print >> sys.stderr,"Error!ALPS Directory '%s' does not exists!" % self.fullCodebase
            sys.exit(2)
        os.chdir(self.fullCodebase)
        self.build("bm_new","TARGET_BUILD_VARIANT=user,DUMP_COMP_BUILD_INFO=true,WITH_DEXPREOPT=false") 
        self.codestripping(self.fullCodebase)

    def verifyBuild(self):
        if not os.path.exists(self.verifyCodebase):
            print >> sys.stderr,"Error!ALPS Directory '%s' does not exists!" % self.verifyCodebase
            sys.exit(2)
        os.chdir(self.verifyCodebase)
        self.build("bm_new","")
        if self.project != "generic" and self.project != "banyan_addon":
            self.build("cts","")
            self.build("otapackage","")
            self.build("sdk","")

    def copyLog(self,path):
        srcPath = os.path.join(path,'out/target/product')
        destPath = os.path.join(self.destPath,'log')
        if not os.path.exists(destPath):
            os.system("mkdir -p %s" % destPath)
        if os.path.exists("%s/*" % destPath):
            os.system("rm %s/%s_*.log*" % (destPath,self.project))
        os.system("cp -f %s/%s_*.log* %s/" % (srcPath,self.project,destPath))

    def checkLog(self):
        currentDir = os.getcwd()
        logPath = os.path.join(currentDir,"out/target/product")
        print >> sys.stdout,"check log path: %s" % logPath
        lists = map(lambda x:x.rstrip(),list(os.popen("find %s -name '*_err' -exec dirname {} \\;" % logPath)))
        if lists:
            return True
        else:
            return False

    def customRelease(self):
        relXml = os.path.join(self.fullCodebase,"mediatek/config/common/releasepackage",self.policy)
        googleVersion = self.readGoogleVersion()
        google_p4 = os.path.join(googlePureSource_p4,googleVersion)
        google_lsf = os.path.join(googlePureSource_lsf,googleVersion)
        print >> sys.stdout,"self.fullCodebase=%s relXml=%s" % (self.fullCodebase,relXml)
        if not os.path.exists(relXml):
            print >> sys.stderr,"the input xml '%s' does not exist!" % relXml
            sys.exit(1)
        if not os.path.exists(self.verifyCodebase):
            os.system("mkdir -p %s" % self.verifyCodebase)
        if not os.path.exists(googlePureSource_lsf):
            os.system("mkdir -p %s" % googlePureSource_lsf)
        if not os.path.exists(os.path.join(google_lsf,'build/core/version_defaults.mk')):
            os.system("mkdir -p %s" % google_lsf)
            print >> sys.stdout,"rsync -a --chmod=a+rwx --delete --force %s/ %s" % (google_p4,google_lsf)
            os.system("rsync -a --chmod=a+rwx --delete --force %s/ %s" % (google_p4,google_lsf))
        print >> sys.stdout,"rsync -a --chmod=a+rwx --delete --force %s/ %s" % (google_lsf,self.verifyCodebase)
        os.system("rsync -a --chmod=a+rwx --delete --force %s/ %s" % (google_lsf,self.verifyCodebase))
        os.chdir(self.fullCodebase)
        os.system("chmod 775 mediatek/build/tools/customRelease.py")
        print >> sys.stdout,"./mediatek/build/tools/customRelease.py ./ ../../verify/alps/ %s %s" % (self.project,relXml)
        os.system("./mediatek/build/tools/customRelease.py ./ ../../verify/alps/ %s %s" % (self.project,relXml))

    def release(self):
        destInhouse = os.path.join(self.destPath,"inhouse/alps")
        destGpl = os.path.join(self.destPath,"gpl/alps")
        if not os.path.exists(destInhouse):
            os.system("mkdir -p %s" % destInhouse)
        if not os.path.exists(destGpl):
            os.system("mkdir -p %s" % destGpl)
        if self.project != "generic" and self.project != "banyan_addon":
            os.chdir(self.verifyCodebase)
            os.system("./makeMtk %s clean" % self.project)
            os.system("rm -rf out/")
            print >> sys.stdout,"rsync -a --chmod=a+rwx --delete --force %s/ %s" % (self.verifyCodebase,destInhouse)
            os.system("rsync -a --chmod=a+rwx --delete --force %s/ %s" % (self.verifyCodebase,destInhouse))
            gplPuller = os.path.join(self.fullCodebase,"mediatek/build/tools/gplpuller.sh")
            os.system("chmod 775 %s" % gplPuller)
            os.system("%s %s %s" % (gplPuller,destInhouse,destGpl))
        else:
            prj_productfile = os.path.join("build/target/product", "%s.mk" % self.project)
            prj_config = os.path.join("mediatek/config", "%s/" % self.project)
            prj_customerfolder = os.path.join("mediatek/custom", "%s/" % self.project)
            prj_vendor = os.path.join("vendor/mediatek/%s/artifacts/" % self.project)
            dest_prj_product = os.path.join(destGpl, "build/target/product")
            dest_prj_config = os.path.join(destGpl, prj_config)
            dest_prj_customerfolder = os.path.join(destGpl, prj_customerfolder)
            dest_prj_vendor = os.path.join(destGpl, prj_vendor)
            if not os.path.exists(dest_prj_product):
                os.system("mkdir -p %s" % dest_prj_product)
            if not os.path.exists(dest_prj_config):
                os.system("mkdir -p %s" % dest_prj_config)
            if not os.path.exists(dest_prj_customerfolder):
                os.system("mkdir -p %s" % dest_prj_customerfolder)
            if not os.path.exists(dest_prj_vendor):
                os.system("mkdir -p %s" % dest_prj_vendor)
            os.system("rsync -a --chmod=a+rwx --delete --force %s %s" % (os.path.join(self.verifyCodebase, prj_productfile), os.path.join(destGpl, prj_productfile)))
            os.system("rsync -a --chmod=a+rwx --delete --force %s %s" % (os.path.join(self.verifyCodebase, prj_config), os.path.join(destGpl, prj_config)))
            os.system("rsync -a --chmod=a+rwx --delete --force %s %s" % (os.path.join(self.verifyCodebase, prj_customerfolder), os.path.join(destGpl, prj_customerfolder)))
            os.system("rsync -a --chmod=a+rwx --delete --force %s %s" % (os.path.join(self.verifyCodebase, prj_vendor), os.path.join(destGpl, prj_vendor)))

    def hostName(self):
        sys = os.name
        if sys == 'nt':
            hostname = os.getenv('computername')
            return hostname
        elif sys == 'posix':
            host = os.popen('echo $HOSTNAME')
            try:
                hostname =host.read()
                return hostname
            finally:
                host.close()
        else:
            return 'Unknow hostname'

    def readGoogleVersion(self):
        version = ""
        versionFile = os.path.join(self.fullCodebase,"build/core/version_defaults.mk")
        if not os.path.exists(versionFile):
            print >> sys.stderr,"Error!the google default version file '%s' does not exist!" % versionFile
            sys.exit(1)
        pattern = re.compile("\s*PLATFORM_VERSION\s*:=\s*(\S+)\s*")
        output = open(versionFile,"r")
        for oput in output.readlines():
            match = pattern.match(oput)
            if match:
                version = match.group(1)
        output.close()
        return 'android-'+version+'_r1'

    def codestripping(self,path):
        tool = os.path.join(self.fullCodebase,"mediatek/build/tools/codestrip.sh")
        if os.path.exists(tool):
            os.system("chmod 775 %s" % tool)
            os.system("%s %s" % (tool,path))
        else:
            print >> sys.stderr,"the code stripping tool '%s' does not exist!" % tool
            sys.exit(1)

    def userSync(self,srcPath,destPath):
        cmd = 'rsync -a --chmod=a+rwx --delete --force %s %s' % (srcPath,destPath)
        mResponse = {'Password:': 'buaa00\n'}
        #pexpect.run(cmd)
        r = pexpect.run('su mtk80206 -c "' + cmd + '"', events=mResponse)
        print r

    def lsfRun(self):
        lsfscript = os.path.join(curDir,"verifyService.py")
        print >> sys.stdout, "LSF Dir = %s" % os.getcwd()
        #print >> sys.stdout, "bsub -q rd5q -I %s" % lsfscript
        #os.system("bsub -q rd5q -I %s" % lsfscript)
        #os.system("bsub -q rd5q -I %s lsf %s %s" % (lsfscript,src,prj))

class lsfService(object):
    def __init__(self):
        """ variable initialization """

    def run(self):
        pass


def main():
    verify = VerifyService()
    verify.getCodebase()
    verify.fullBuild()
    verify.customRelease()
    verify.verifyBuild()
    verify.release()
#    verify.clean()
    print >> sys.stdout,"[SERVER]: %s" % verify.hostName()
main()

