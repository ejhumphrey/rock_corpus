#!/usr/bin/perl

# Takes two analysis files, and calculates the total time that they agree.
#
# You can compare chromatic roots, absolute roots,
# or key, depending on the value of $cf ("compared feature"). If $cf = 3 you're comparing
# chromatic roots; $cf = 5, you're comparing keys; $cf = 6, you're comparing absolute roots.
# 
# START END ROMNUM CROOT DROOT KEY AROOT
# 0     1   2      3     4     5   6 
#
# Run it like this:
# ./compare.pl [input-file1] [input-file2]

$cf = 3;

$v = 0;    # If $v=1, outputs parallel chord lists indicating differences; if $v=0, just outputs num mm. and num mm. different

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
$numchords1 = $i;

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
$numchords2 = $i;

if($v > 0) {
    printf("Numchords: %d %d\n", $numchords1, $numchords2);
}

$c1 = $c2 = 0;
$prev_earliest = -1;

if($a1[0][0] != $a2[0][0]) {
    printf("Error: Two files do not have the same start times\n");
    exit;
}

# printf("End time 1 = %.2f; end time 2 = %.2f\n", $a1[$numchords1-1][1], $a2[$numchords2-1][1]);

if($a1[$numchords1-1][1] != $a2[$numchords2-1][1]) {
    printf("Error: Two files do not have the same end times (%.3f, %.3f)\n", $a1[$numchords1-1][1], $a2[$numchords2-1][1]);
    $adj = 1;
    if($v==0) {
	exit;
    }
    else {
	printf("Setting end times equal for comparison:\n");
	if($a1[$numchords1-1][1] > $a2[$numchords2-1][1]) {
	    $a2[$numchords2-1][1] = $a1[$numchords1-1][1];
	}
	else {
	    $a1[$numchords1-1][1] = $a2[$numchords2-1][1];
	}
    }
}


while() {

# $c1 is the next chord in file1, $c2 in file2. $r1 is the current relative root (or chromatic
# root or key) in file1, $r2 in file2.
# $prev_earliest is the most recent timepoint in either file. See whether $c1 or $c2 is earliest
# (or whether they're simultaneous) and define this timepoint as $earliest. Then create a span
# ($prev_earliest, $earliest) and check whether $r1 = $r2 for that span.
# $e = 1 means new chord in file1 (redefine $r1 after comparing), $e = 2 means new chord in
# file2, $e = 3 means new chord in both files, $e = 3 means we're at the end of both lists.

    if($c1 == $numchords1 && $c2 == $numchords2) {
        # We've reached the end of both chord lists
	$earliest = $a1[$c1-1][1];
	$e = 3;
    }
    elsif($c2 == $numchords2) {
        # We've reached the end of list 2
	$earliest = $a1[$c1][0];
	$e = 1;
    }
    elsif($c1 == $numchords1) {
        # We've reached the end of list 1
	$earliest = $a2[$c2][0];
	$e = 2;
    }
    elsif($a1[$c1][0] < $a2[$c2][0]) {
        # The earliest new chord is in list 1
	#printf("Earliest is %5.2f (%d %d)\n", $a1[$c1][0]);
	$earliest = $a1[$c1][0];
	$e = 1;
    }
    elsif($a2[$c2][0] < $a1[$c1][0]) {
        # The earliest new chord is in list 2
	$earliest = $a2[$c2][0];
	$e = 2;
    }
    else {
        # The two chords are tied for earliest
	$earliest = $a1[$c1][0];
	$e = 0;
    }

    if($prev_earliest != -1) {
	if($v > 0) {
	    printf("%5.2f-%5.2f (%d): %d %d ", $prev_earliest, $earliest, $e, $r1, $r2);
	    if($r1 != $r2) {
		printf("* ");
	    }
	    printf("\n");
	}
	$total_time += $earliest - $prev_earliest;
	if($r1 == $r2) {
	    $total_ag += $earliest - $prev_earliest;
	}
    }

    if($e == 3) {
	last;
    }
    
    $prev_earliest = $earliest;

    # Use $a1[$c1][3] for relative (chromatic) roots, $a1[$c1][6] for absolute roots, $a1[$c1][5] for keys. We control this with $cf, set at the top of the code.

    if($e == 1 || $e == 0) {
	$r1 = $a1[$c1][$cf];
	$c1++;
    }
    if($e == 2 || $e == 0) {
	$r2 = $a2[$c2][$cf];
	$c2++;
    }

}

if($v > 0) {
    if($adj == 1) {
	printf("WARNING: End times of files did not match. Figure below reflect comparison after end time adjustment.\n");
    }
    printf("Total time = %5.2f; time in agreement = %5.2f; proportion = %5.3f\n", $total_time, $total_ag, $total_ag / $total_time);
}
else {
    printf("%.2f %.2f (%.3f)\n", $total_time, $total_ag, $total_ag / $total_time);
}
