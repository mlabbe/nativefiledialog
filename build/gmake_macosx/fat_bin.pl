#!/usr/bin/perl -w

use strict;
use Cwd;

# run this script to compile native file dialog for both arm64 and
# x64 architectures, and to combine them into one static, fat
# binary.

sub run {
    my $cmd = shift;

    print("> $cmd\n");
    system($cmd);

    if ($? == -1) {
        die "Shell command failed";
    }
}


if ($#ARGV == -1) {
    print "Usage: $0 <debug|release>\n";
    exit 1;
}

my $cfg = $ARGV[0];
my $ext = '';
if ($cfg eq 'debug') {
    $ext = '_d';
}


run("make config=${cfg}_x64 nfd clean");
run("make config=${cfg}_arm64 nfd clean");

run("make config=${cfg}_x64 target=all verbose=1 nfd");
run("make config=${cfg}_arm64 target=all verbose=1 nfd");

my $outfile = "../lib/$cfg/libnfd$ext.a";
run("lipo " .
    "../lib/$cfg/arm64/libnfd$ext.a " .
    "../lib/$cfg/x64/libnfd$ext.a " .
    "-create " .
    "-output $outfile");

my $abs_outfile = Cwd::abs_path($outfile);
print("multi-arch binary $abs_outfile successfully created.\n");

