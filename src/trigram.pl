#!/usr/bin/perl
#
# Takes an analysis file, and extracts "trigrams" - groups of three adjacent chromatic roots.
# See theory.esm.rochester.edu/rock_corpus for more info.
#
# START END ROMNUM CROOT DROOT KEY AROOT
# 0     1   2      3     4     5   6 

open(INFILE, $ARGV[0]);

while(<INFILE>) {   
    $line = $_;
    chomp($line);
    $line =~ s/^[ ]+//;
    @words = split(/[ ]+/, $line);

    if($words[0] eq "%") {
	# printf("%s\n", $words[1]);
	next;
    }

    if($words[0] eq "---") {
	$c=0;
    }

    else {
	if($c >= 2) {
	    if($words[5] == $prev_key && $prev_key == $second_prev_key) {
		printf("%d %d %d\n", $second_prev_croot, $prev_croot, $words[3]);
	    }
	}
	if($c > 0) {
	    $second_prev_croot = $prev_croot;
	    $second_prev_key = $prev_key;
	}
	$prev_croot = $words[3];
	$prev_key = $words[5];
	$c++;
    }
}

