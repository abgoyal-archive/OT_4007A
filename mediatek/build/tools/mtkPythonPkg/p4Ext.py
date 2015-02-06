#! /usr/bin/python

import sys
import os
import smtplib
import config
from optparse import OptionParser

debug = False

class Perforce(object):
    def __init__(self):
        """ variable initialization """
        self.p4user = config.p4['P4USER']
        self.p4port = config.p4['P4PORT']
        self.p4client = config.p4['P4CLIENT']
        self.p4char = config.p4['P4CHAR']
        self.p4password = config.p4['P4PASSWORD']
        os.system("echo %s > 'p4login'" % self.p4password)

    def Login(self):
        os.putenv("P4USER",self.p4user)
        os.putenv("P4PORT",self.p4port)
        os.putenv("P4CLIENT",self.p4client)
        os.putenv("P4CHAR",self.p4char)
        try:
            os.system("p4 login < 'p4login'")
	    os.system("rm -f p4login")
            return True
        except Exception, e:
            print >> sys.stderr,"[ERROR: p4Login] %s" % str(e)
            return False

    def GetCheckinOwner(self,path):
        if not os.path.exists(path):
            print >> sys.stderr,"[ERROR: GetCheckinOwner] '%s' does not exists!" % path
            sys.exit(1)
        if os.path.isdir(path):
            path = os.path.join(path,'...')
        cmd = "p4 changes %s | cut -d ' ' -f6 | cut -d @ -f1 | sort | uniq | grep -v 'yang.yang'" % path
        try:
            ps = os.popen(cmd,'r')
            return ps.read()
        except Exception, e:
            print >> sys.stderr,"[ERROR: GetCheckinOwner] %s" % str(e)
            return False

    def GetLatestRevison(self,path):
	if not os.path.exists(path):
            print >> sys.stderr,"[ERROR: GetLatestRevison] '%s' does not exists!" % path
            sys.exit(1)
        if os.path.isdir(path):
            path = os.path.join(path,'...')
	cmd = "p4 sync %s" % path
        try:
            ps = os.popen(cmd,'r')
            return ps.read()
        except Exception, e:
            print >> sys.stderr,"[ERROR: GetLatestRevison] %s" % str(e)
            return False

    def debug(self):
        self.Login()

if debug == True:
    p4 = Perforce()
    p4.debug()
