#!usr/bin/perl
my $configFile = $ARGV[0];
my $DRM_VALUE = $ARGV[1];
my $KEY_VALUE = $ARGV[2];
die "the file $configFile is NOT exsit\n" if ( ! -e $configFile);

open FILE, "<$configFile";
my @lines;
while (<FILE>) {
	print "$_";
	if (/^(\S+)\s*=\s*(\S+)/) {
		if(!($1 eq "$KEY_VALUE")){
			@lines=(@lines,$_);
		}
		
	}else{
		@lines=(@lines,$_);
	}
}
close FILE;

my $filedir = "mediatek/config/out/$FULL_PROJECT";
my $write_filename = "$filedir/ProjectConfigtmp.mk";

system("touch $write_filename") if (!-e $write_filename);
system("chmod u+w $write_filename") if (-e $write_filename);
system("mkdir -p $filedir") if ( ! -d "$filedir");
die "can NOT open $write_filename:$!" if ( ! open OUT_FILE, ">$write_filename");

foreach my $option (@lines) {	
	print OUT_FILE "$option";
}
print OUT_FILE "$KEY_VALUE = $DRM_VALUE\n";
close OUT_FILE;

system("mv $write_filename $configFile");
