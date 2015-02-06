#!/usr/bin/python
import sys,os
import re
import getopt
import subprocess
from subprocess import *
from optparse import OptionParser
from os.path import join, getsize

def build_search_list(folder,path):
    list = []
    for src in os.listdir(folder):
        srcpath = os.path.join(folder,src)        
        srcpath = srcpath + "/"+ path
        for root, dirs, files in os.walk(srcpath):
            for file in files:           
                list.append(file)                        
    temp = set(list)
    list_temp=[]
    for i in temp:
        list_temp.append(i)
    return list_temp


def check_local_module_package(file):
    f_input=open(file)
    for line in f_input:
        if re.search('LOCAL_PACKAGE_NAME',line):
            f_input.close()
            return True
    f_input.close()
    return False

def check_exist(list, listmdpi):
    for li in listmdpi:
        if li in list:
            list.remove(li)
    return list
    
def find_mapping_res(folder):
    list = build_search_list(folder,"drawable-hdpi")
    templist = build_search_list(folder,"drawable-hdpi")  
    listmdpi = build_search_list(folder,"drawable-mdpi")
    listhdpifinger = build_search_list(folder,"drawable-hdpi-finger")
    listhdpifingertemp = build_search_list(folder,"drawable-hdpi-finger")
    listmdpifinger = build_search_list(folder,"drawable-mdpi-finger") 
    if len(list)>0:
        package_name = find_process_folder_packagename(folder)
        list = check_exist(list, listmdpi)        
        if len(list) > 0:
            total = len(list)
            for li in list:
                print '[Check Res][Incomplete Res][no mdpi Res] ' \
                            + package_name \
                            + ' ' +folder.split(os.getcwd(),1)[0]+'drawable-hdpi/' +li 
            print '[Check Res][Incomplete Res][no mdpi Res Sum] ' \
                + package_name \
                + ' %d'%(total)
    if len(listmdpi)>0:
        package_name = find_process_folder_packagename(folder)
        listmdpi = check_exist(listmdpi, templist)         
        if len(listmdpi) > 0:
            total = len(listmdpi)
            for litemp in listmdpi:
                print '[Check Res][Incomplete Res][no hdpi Res] ' \
                            + package_name \
                            + ' ' +folder.split(os.getcwd(),1)[0]+'drawable-mdpi/'+ litemp 
            print '[Check Res][Incomplete Res][no hdpi Res Sum] ' \
                + package_name \
                + ' %d'%(total)
    if len(listhdpifinger)>0:
        package_name = find_process_folder_packagename(folder)
        listhdpifinger = check_exist(listhdpifinger, listmdpifinger)         
        if len(listhdpifinger) > 0:
            total = len(listhdpifinger)
            for litemp in listhdpifinger:
                print '[Check Res][Incomplete Res][no mdpi-finger Res] ' \
                            + package_name \
                            + ' ' +folder.split(os.getcwd(),1)[0]+'drawable-hdpi-finger/'+litemp 
            print '[Check Res][Incomplete Res][no mdpi-finger Res Sum] ' \
                + package_name \
                + ' %d'%(total)
    if len(listmdpifinger)>0:
        package_name = find_process_folder_packagename(folder)
        listmdpifinger = check_exist(listmdpifinger, listhdpifingertemp)         
        if len(listmdpifinger) > 0:
            total = len(listmdpifinger)
            for litemp in listmdpifinger:
                print '[Check Res][Incomplete Res][no hdpi-finger Res] ' \
                            + package_name \
                            + ' '  +folder.split(os.getcwd(),1)[0]+'drawable-mdpi-finger/'+ litemp 
            print '[Check Res][Incomplete Res][no hdpi-finger Res Sum] ' \
                + package_name \
                + ' %d'%(total)                    
def find_process_folder(dir):
    folder_list=[]
    for root, dirs, files in os.walk(dir):
        for file in files:
            if re.search('Android\.mk',file):
                if check_local_module_package(join(root,file)):
                    folder_list.append(root)
    return folder_list
    
def main():
    parser = OptionParser()
    parser.add_option("-d","--dir",action = "store",type = "string",dest = "dir")
    
    (options, args) = parser.parse_args()
    if options.dir:
        folder_list = find_process_folder(options.dir)
        for folder in folder_list:
            #print 'Process folder : ' + li
            find_mapping_res(folder)
    else:
        print "the process directory not assign !!!"

def find_process_folder_packagename(dir):
    folder_list=[]
    for root, dirs, files in os.walk(dir):
        for file in files:
            if re.search('Android\.mk',file):
                return check_local_module_package_packagename(join(root,file))
  #  return folder_list

def check_local_module_package_packagename(file):
    f_input=open(file)
    for line in f_input:
        if re.search('LOCAL_PACKAGE_NAME',line):
            f_input.close()
            return line[22:].rstrip('\n')
    f_input.close()



if __name__ == "__main__":
    main()
