#!usr/bin/perl
my $prj = $ARGV[0];
my $dir = "out/target/product/$prj";
my $cfg = "mediatek/custom/$prj/security/image_auth/IMG_AUTH_CFG.ini";
my $key = "mediatek/custom/$prj/security/image_auth/IMG_AUTH_KEY.ini";
`mkdir $dir/signed_bin` if ( ! -d "$dir/signed_bin" );
print "Image Dir '$dir'\n";
my $command = "mediatek/build/tools/SignTool/SignTool.sh";

##########################################################
# Process Common Files
##########################################################

my @imgs_need_sign = ("boot.img", "logo.bin", "recovery.img", "secro.img", "system.img", "uboot_${prj}.bin", "userdata.img");
foreach my $img (@imgs_need_sign) {
	push (@miss_img, $img) if ( ! -e "$dir/$img");
}
die "@miss_img\nall the imgs above is NOT exsit\n" if (@miss_img > 0);

foreach my $img (@imgs_need_sign) {
	if ( ! -e "$dir/$img") {
		warn "the $img is NOT exsit, please check\n";
		next;
	}
	my $signed_img = $img;
	$signed_img =~ s/\./-sign\./;
	print "Sign Image '$dir/$img' ...\n";
	&print_system("$command $key $cfg $dir/$img $dir/signed_bin/$signed_img");
}

sub print_system {
	my $command = $_[0];
	my $rslt = system($command);
	print "$command: $rslt\n";
	die "Failed to execute $command" if ($rslt != 0);
}

##########################################################
# Process EMMC Files
##########################################################
my @imgs_need_sign = ("MBR", "EBR1", "EBR2");

foreach my $img (@imgs_need_sign) {
	if (-e "$dir/$img") {		
		print "Sign Image '$dir/$img' ...\n";
		&print_system("$command $key $cfg $dir/$img $dir/signed_bin/${img}-sign");
	}
}

my @imgs_need_sign = ("cache.img");

foreach my $img (@imgs_need_sign) {
	if (-e "$dir/$img") {		
		my $signed_img = $img;
		$signed_img =~ s/\./-sign\./;
		print "Sign Image '$dir/$img' ...\n";
		&print_system("$command $key $cfg $dir/$img $dir/signed_bin/$signed_img");
	}
}