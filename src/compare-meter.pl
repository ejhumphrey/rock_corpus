#!/usr/bin/perl

# Takes two analysis files, of the format (m. number) (timesig), and checks if they agree perfectly.
# See www.theory.esm.rochester.edu/rock_corpus for more info.

open(FILE1, $ARGV[0]) or die "Can't open $ARGV[0]\n";
open(FILE2, $ARGV[1]) or die "Can't open $ARGV[1]\n";

$adj = 0;
$i=0;

while(<FILE1>) {      
    $line = $_;
    chomp($line);
    $line =~ s/^[ ]+//;
    if($line eq "---" || $line eq "") {
	next;
    }
    elsif($line =~ /^Error/) {
	printf("Error: First file did not parse\n");
	exit;
    }
    else {                                   # Create 2-D array, $a1[row][column]
	@words = split(/[ ]+/, $line);
	push @a1, [ @words ];
	$i++;
    }
}
$num_measures1 = $i;

$i=0;
while(<FILE2>) {      
    $line = $_;
    chomp($line);
    $line =~ s/^[ ]+//;
    if($line eq "---") {
	next;
    }
    elsif($line =~ /^Error/) {
	printf("Error: Second file did not parse\n");
	exit;
    }
    else {
	@words = split(/[ ]+/, $line);
	push @a2, [ @words ];
	$i++;
    }
}
$num_measures2 = $i;

if($num_measures1 != $num_measures2) {
    printf("Measure counts do not agree (%d, %d)\n", $num_measures1, $num_measures2);
    exit;
}

if($v > 0) {
    printf("Numchords: %d %d\n", $numchords1, $numchords2);
}

$mismatch_found = 0;

for($m=0; $m<$num_measures1; $m++) {

    if($a1[$m][1] != $a2[$m][1]) {
	printf("Mismatch on m. %d (%d, %d)\n", $m, $a1[$m][1], $a2[$m][1]);
	$mismatch_found++;
    }
}

if($mismatch_found == 0) {
    printf("OK\n");
}

