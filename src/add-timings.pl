#!/usr/bin/perl

# Takes a chord list or note list, as well as a list of absolute measure times, and adds absolute timing info to the chord/note
# list. First argument should be 0 for a chord list, 1 for a note list.

$input_mode = $ARGV[0];

if(!(open(EVENTLIST, $ARGV[1]))) {
    printf("File '%s' not found\n", $ARGV[1]);
    exit;
}
if(!(open(TIMINGS, $ARGV[2]))) {
    printf("File '%s' not found\n", $ARGV[2]);
    exit;
}

$c=$n=0;

if ($input_mode==0) {
    while(<EVENTLIST>) {   
	chomp($_);
	if($_ eq "---") {
	    last;
	}
	$_ =~ s/^[ ]+//g;
	@words = split(/[ ]+/, $_);
	push @chords, [ @words ];
	$c++;
    }
    $numchords = $c;
}
if ($input_mode==1) {
    while(<EVENTLIST>) {   
	chomp($_);
	if($_ eq "---") {
	    last;
	}
	$_ =~ s/^[ ]+//g;
	@words = split(/[ ]+/, $_);
	push @notes, [ @words ];
	$n++;
    }
    $numnotes = $n;
}

$t=0;
$first=1;

while(<TIMINGS>) {   
    chomp($_);
    @twords = split(/\t/, $_);
    if($first==1) {
	if($twords[1] == 0) {
	    $upbeat = 0;
	}
	else {
	    $upbeat = 1;
	}
	$first = 0;
    }
    $mtime[$t] = $twords[0];
    # printf("%.3f\n", $mtime[$t]);
    $t++;
}
$num_measures = $t;

if($upbeat==1) {
    for($t=$num_measures; $t>=1; $t--) {
	$mtime[$t] = $mtime[$t-1];
    }
    $mtime[0] = $mtime[1] - ($mtime[2] - $mtime[1]);
    #printf("Upbeat found; first downbeat estimated at %.3f\n", $mtime[0]);
    $num_measures++;
}

if($input_mode == 0) {

    for($c=0; $c<$numchords; $c++) {
	
	for($m=0; $m<$num_measures; $m++) {
	    if($m >= $chords[$c][0]) {
		last;
	    }
	}
	if($m == $num_measures) {
	    printf("Error: Chord starts after last measure barline (%d)\n", $m-1);
	    exit;
	}
	$m1 = $m;
	for($m=0; $m<$num_measures; $m++) {
	    if($m >= $chords[$c][1]) {
		last;
	    }
	}
	if($m == $num_measures) {
	    printf("Error: Chord ends after last measure barline (%d)\n", $m-1);
	    exit;
	}
	$m2 = $m;
	
	if($chords[$c][0] == $m1) {
	    # The m1 barline is simultaneous with the chord start
	    $cstart = $mtime[$m1];
	}
	else {
	    # The m1 barline is after the chord start
	    $mlength = $mtime[$m1] - $mtime[$m1-1];
	    $cstart = $mtime[$m1-1] + (($chords[$c][0] - ($m1-1)) * $mlength);
	}
	if($chords[$c][1] == $m2) {
	    $cend = $mtime[$m2];
	}
	else {
	    $mlength = $mtime[$m2] - $mtime[$m2-1];
	    $cend = $mtime[$m2-1] + (($chords[$c][1] - ($m2-1)) * $mlength);
	}
	printf("%.3f\t%.2f\t%s\t%d\t%d\t%d\t%d\n", $cstart, $chords[$c][0], $chords[$c][2], $chords[$c][3], $chords[$c][4], $chords[$c][5], $chords[$c][6]);
	if($c == $numchords-1) {
	    printf("%.3f\t%.2f\tEnd\n", $cend, $chords[$c][1]);
	}
    }

}

if($input_mode == 1) {

    for($n=0; $n<$numnotes; $n++) {
	
	for($m=0; $m<$num_measures; $m++) {
	    if($m >= $notes[$n][0]) {
		last;
	    }
	}
	if($m == $num_measures) {
	    printf("Error: Note starts after last measure barline (%d)\n", $m-1);
	    exit;
	}
	$m1 = $m;
	
	if($notes[$n][0] == $m1) {
	    $nstart = $mtime[$m1];
	}
	else {
	    $mlength = $mtime[$m1] - $mtime[$m1-1];
	    $nstart = $mtime[$m1-1] + (($notes[$n][0] - ($m1-1)) * $mlength);
	}
	printf("%.3f\t%5.2f\t%d\t%d\n", $nstart, $notes[$n][0], $notes[$n][1], $notes[$n][2]);
    }
    
}

