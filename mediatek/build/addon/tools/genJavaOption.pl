#!/usr/bin/perl -w
use strict;
use File::Basename;

&usage if (($#ARGV == 0) && ($ARGV[0] =~ /^-h|-help|--help$/));
&usage if ($#ARGV < 3);

my ($infoLog, $comDefsFile, $androidDefsFile, $javaOptFile, $DEBUG) = @ARGV;

# disable debug mode in default
$DEBUG = 0 if (!defined $DEBUG);

my @enOptions = ();
my @disOptions = ();

#
# main flow
#

# get enabled options
@enOptions  = getEnOpts($infoLog);

# get disable options
@disOptions = getDisOpts(\@enOptions, [getAllOpts($comDefsFile, $androidDefsFile)]);

# generate JAVA code feature option class file
genJavaOptDefsFile(\@enOptions, \@disOptions, $javaOptFile);

#
# subroutines
#

# read info.log to get enabled options
sub getEnOpts
{
  my $log = shift;
  my @enOpts = ();
  open(INFOLOG, "<$log") or die "can NOT open \"$log\"!\n";
  while(<INFOLOG>)
  {
    if (/^\s*COM_DEFS\s*=\s*((\w+(=\w+)?\s+)*\w+(=\w+)?\s*)/)
    {# match the compile options in COM_DEFS
      @enOpts = split(/\s+/, $1);
      last;
    }
    else
    {
      next;
    }
  }
  close(INFOLOG);
  return sort(@enOpts);
}

# parse definition makefile to read common options
sub readComDefs
{
  my $defsFile  = shift;
  my @comDefs   = ();
  my $optionStr = "";
  my $lineNo    = 0;
  my $isOptSection = 0;

  open(DEFSFILE, "<$defsFile") or die "can NOT open \"$defsFile\"!\n";
  while(<DEFSFILE>)
  {
    $lineNo++;
    my $line = $_;
    chomp $line;
    if ($line =~ /^\s*COM_DEFS\s*\+?:?=\s*((\w+(=\w+)?\s+)*(\w+(=\w+)?\s*)?(\\?))$/)
    { # match the line contains "COM_DEFS :=" or "COM_DEFS =" or "COM_DEFS +="
      print "[",basename($defsFile),"]","Matched line: $lineNo: ",$line,"\n" if ($DEBUG);
      # inside of option section & enable option section flag if "\" occurs
      $isOptSection = 1 if ($6 ne "");
      $optionStr .= " ".$1;
    }
    elsif ($isOptSection && $line =~ /^\s*((\w+(=\w+)?\s+)*(\w+(=\w+)?\s*)?(\\?))$/)
    { # inside of option section
      print "[",basename($defsFile),"]","Matched line: $lineNo: ",$line,"\n" if ($DEBUG);
      $isOptSection = 0 if ($6 eq "");
      $optionStr .= " ".$1;
    }
    else
    { # outside of option section & disable option section flag
      print "[",basename($defsFile),"]","NOT matched line: $lineNo: ",$line,"\n" if ($DEBUG);
      $isOptSection = 0;
      next;
    }
  }
  close(DEFSFILE);

  $optionStr =~ s/\\/ /g; # replace '\' with ' '
  $optionStr =~ s/^\s+(.*)\s+$/$1/; # strip option string(remove the blanks in begin/end of the string)
  @comDefs = sort(split(/\s+/, $optionStr));

  print "[raw data] All Compile Options in ",
        "[",
        basename($defsFile),
        "]",
        ": ",
        join(" ", sort(@comDefs)),
        "\n" 
  if ($DEBUG);

  # remove the options which occurs duplicated in definition files
  rmDuplicatedOpt(\@comDefs);

  print "[refined data] All Compile Options in ",
        "[",
        basename($defsFile),
        "]",
        ": ",
        join(" ", sort(@comDefs)),
        "\n" 
  if ($DEBUG);

  return sort(@comDefs);
}


# to get all options
sub getAllOpts
{
  my ($comDefsMF, $androidDefsMF) = @_;
  my @allOpts = ();

  push(@allOpts, readComDefs($comDefsMF));
  push(@allOpts, readComDefs($androidDefsMF));

  # remove the options which occurs duplicated in common & android definition files
  rmDuplicatedOpt(\@allOpts);

  print "[refined data] All Compile Options in ",
        "[",
        basename($comDefsMF),
        "]",
        " and ",
        "[",
        basename($androidDefsMF),
        "]",
        ": ",
        join(" ", sort(@allOpts)),
        "\n" 
  if ($DEBUG);

  return sort(@allOpts);
}

# get disabled options
sub getDisOpts
{
  my ($enOpts_ref, $allOpts_ref) = @_;
  my @disOpts = ();

  foreach my $e (@$allOpts_ref)
  {
    my $tmp = quotemeta($e);
    push(@disOpts, $e) if (!grep(/^$tmp$/, @$enOpts_ref));
  }
  return sort(@disOpts);
}

# gen FeatureOption class which JAVA codes used
sub genJavaOptDefsFile
{
  my ($enOpts_ref, $disOpts_ref, $classFile) = @_;

  my $fileHeader = <<__FILEHEADER__;
/* Automatically generated class for JAVA codes use(enable/disable feature), do NOT edit */

package com.mediatek.featureoption;
__FILEHEADER__

  my $classHeader = <<__CLASSHEADER__;
public final class FeatureOption
__CLASSHEADER__

  my $intendedSpace = "    "; # 4 spaces

  my $javaPublicStaticBooleanVarDeclareKeyWords = "public static final boolean";
  my $javaPublicStaticIntVarDeclareKeyWords = "public static final int";

  open(CLASSFILE, ">$classFile") or die "can NOT open \"$classFile\"!\n";

  # write class file header
  print CLASSFILE $fileHeader,"\n";

  # write FeatureOption class header
  print CLASSFILE $classHeader;

  # write class body begin delimiter
  print CLASSFILE '{',"\n";

  # write enabled options into class file
  my $enOptsComment = <<__ENOPTSCOMMENT__;
$intendedSpace/**
$intendedSpace * enabled options
$intendedSpace */
__ENOPTSCOMMENT__

  print CLASSFILE $enOptsComment;

  foreach my $opt (@$enOpts_ref)
  {
    if ($opt !~ /=/)
    {
      print CLASSFILE $intendedSpace,"$javaPublicStaticBooleanVarDeclareKeyWords $opt=true;","\n";
    }
    else
    {
      print CLASSFILE $intendedSpace,"$javaPublicStaticIntVarDeclareKeyWords $opt;","\n";
    }
  }

  # write disabled options into class file

  my $disOptsComment = <<__DISOPTSCOMMENT__;
$intendedSpace/**
$intendedSpace * disabled options
$intendedSpace */
__DISOPTSCOMMENT__

  print CLASSFILE $disOptsComment;

  foreach my $opt (@$disOpts_ref)
  {
    print CLASSFILE $intendedSpace,"$javaPublicStaticBooleanVarDeclareKeyWords $opt=false;","\n";
    # Todo...
    # process OPTION_NAME=num when it is disabled
    # how to set its value in class?
  }

  # write class body end delimiter
  print CLASSFILE '}',"\n";

  close(CLASSFILE);
}

# remove options which occurs duplicated in definition file
sub rmDuplicatedOpt
{
  my $optionSet_ref = shift;

  foreach my $opt (@$optionSet_ref)
  {
    my $tmp = quotemeta($opt);
    if (grep(/$tmp/, @$optionSet_ref)>1)
    {
      warn "WARNING: option \"$opt\" occurs duplicated in definition file!\n";
      rmElem($opt, $optionSet_ref);
      push(@$optionSet_ref, $opt);
    }
  }
}

# remove an element from an array
sub rmElem
{
  my ($element, $arr_ref) = @_;
  $element = quotemeta($element);
  @$arr_ref = grep(!/^$element$/i, @$arr_ref);
}

# script usage
sub usage
{
  print << "__EOFUSAGE";
usage: $0 infoLog comDefsFile androidDefsFile javaOptFile [DEBUG]

infoLog               info.log which contains enabled compile options dumped in building time
comDefsFile           common definition configuration makefile 
                      (such as, mediatek/build/addon/core/common_defs.mak)
androidDefsFile       android definition configuration makefile 
                      (such as, mediatek/build/addon/core/android_defs.mak)
javaOptFile           JAVA code feature option class file 
                      (such as, 
                       frameworks/base/mediatek/featureoption/java/com/mediatek/featureoption/FeatureOption.java)
DEBUG                 enable switch (1/0), disabled in default
                      1: enable debug mode
                      0: disable debug mode
__EOFUSAGE
  exit 1;
}

