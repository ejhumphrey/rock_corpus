#!/usr/bin/perl

# Takes melodic data with three numbers on each line - sd, abs pc, and
# note length (as produced by process-mel5.pl in output mode 0),
# tallies them up.
# If v == 0: output proportional vector of SD counts. If v == -1: output proportional vector of SD lengths. If v == 1, output more info.

$v = 0;
$v = $ARGV[0];

for($i=0; $i<12; $i++) {
    $sd[$i] = 0;
    $pc[$i] = 0;
    $sdlength[$i] = 0;
}

$dt_files = $tdc_files = $dt_not_found = $tdc_not_found = 0;

while(<STDIN>) {   

    chomp($_);
    if($_ =~ /\#/) {
	$filename = $_;
	if($filename =~ /dt/) {
	    $dt_files++;
	}
	elsif($filename =~ /tdc/) {
	    $tdc_files++;
	}
	next;
    }
    if($_ eq "X") {
	if($filename =~ /dt/) {
	    $dt_not_found++;
	}
	elsif($filename =~ /tdc/) {
	    $tdc_not_found++;
	}
	next;
    }

    @words = split(/ /, $_);
    $numwords = @words;
    if($numwords != 3) {
	printf("Bad input: %s\n", $_);
	# exit;
	next;
    }

    $sd[$words[0]]++;
    $pc[$words[1]]++;
    $sdlength[$words[0]] += $words[2];

}

if($v > 0) {
    printf("%d DT files (%d not found), %d TdC files (%d not found)\n", $dt_files, $dt_not_found, $tdc_files, $tdc_not_found);
}
 
$total_sds = 0;
for($i=0; $i<12; $i++) {
    $total_sds += $sd[$i];
    $total_length += $sdlength[$i];
}

$total_pcs = $total_sds;

if($v > 0) {
    printf("Scale-degrees:\n");
}

if($total_sds > 0) {
    if($v == 0 || $v > 0) {
	for($i=0; $i<12; $i++) {
	    printf("%.3f ", $sd[$i] / $total_sds);
	}
    }
    if($v == -1 || $v > 0) {
	for($i=0; $i<12; $i++) {
	    printf("%.3f ", $sdlength[$i] / $total_length);
	}
    }
    printf("\n");
}

if($v > 0) {
    printf("SD Counts:\n");
    for($i=0; $i<12; $i++) {
	printf("%2d: %d\n", $i, $sd[$i]);
    }
}

if($v > 0) {

    printf("Pitch-classes:\n");
    if($total_pcs > 0) {
	for($i=0; $i<12; $i++) {
	    printf("%.3f ", $pc[$i] / $total_pcs);
	}
	printf("\n");
    }
        
    printf("SD's Ranked:\n");
    
    for($i=0; $i<12; $i++) {
	$done[$i] = 0;
    }
    
    for($i=0; $i<12; $i++) {
	$best = -1;
	$bestscore = -1;
	for($i2=0; $i2<12; $i2++) {
	    if($done[$i2] == 1) {
		next;
	    }
	    if($sd[$i2] > $bestscore) {
		$best = $i2;
		$bestscore = $sd[$i2];
	    }
	}
	printf("%2d: %d\n", $best, $bestscore);
	$done[$best] = 1;
    }
}
    
    

    
