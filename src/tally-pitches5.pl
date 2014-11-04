#!/usr/bin/perl
# START END ROMNUM CROOT DROOT KEY AROOT
# 0     1   2      3     4     5   6 

# Takes chord-list input, extracts the pitch-classes (pcs) from each chord, tallies up the pcs and outputs the
# counts and/or distribution.

# You can use it to get RELATIVE pcs (scale-degrees), or ABSOLUTE pcs. (Search for RELATIVE below)
# You can also count each chord-tone once, or weight them by the duration of the span (look for $dur below).

# $qual: 0 = major, 1 = minor, 2 = aug, 3 = dim
# $sev: 0 = not a seventh, 1 = minor seventh, 2 = major seventh

# alg = 0, count each chord once; alg = 1, weight each chord by its length
$alg = 0;
$alg = $ARGV[0];

# rel = 0, absolute pcs; rel = 1, relative pcs
$rel = 0;
$rel = $ARGV[1];

# verbosity: if 0, just print distribution vector on one line; if 1, print more verbose output; 
# if -1, print binary vector on one line (1 if the pc occurs at all, 0 otherwise)
$v = 0;
$v = $ARGV[2];

for($pc=0; $pc<12; $pc++) {
    $pc_tally[$pc] = 0;
}

while(<STDIN>) {   
    $line = $_;
    chomp($line);
    $line =~ s/^[ ]+//;
    @words = split(/[ ]+/, $line);

    if($words[0] eq "Input") {
	# means the expander couldn't find the input file
	printf("%s\n", $line);
	exit;
    }

    elsif($words[0] eq "%") {
	# printf("%s\n", $words[1]);
	next;
    }

    elsif($words[0] eq "---") {
	next;
    }

    if($alg == 0) {
	$dur = 1;
	# count each chord once (not reported in SMPC paper)
    }
    elsif($alg == 1) {
	# weight each chord by its length
	$dur = $words[1] - $words[0];
    }

    if($rel == 1) {
	$r = $words[3];
    }
    elsif($rel == 0) {
	$r = ($words[6]);
    }

    # If it starts with a capital letter, it's major, unless it contains a, in which case it's augmented
    if($words[2] =~ /^[b\#]?[A-Z]/) {
	if($words[2] =~ /a/) {
	    $qual=2;
	}
	else {
	    $qual=0;
	}
    }
    # If it starts with lower-case, it's minor, unless it contains o/h/x, in which case it's diminished
    else {
	if($words[2] =~ /o|h|x/) {
	    $qual=3;
	}
	else {
	    $qual=1;
	}
    }

    $pc_tally[$r]+=$dur;

    if($qual % 2 == 0) {
	$pc_tally[($r+4) % 12]+=$dur;
    }
    else {
	$pc_tally[($r+3) % 12]+=$dur;
    }

    if($qual < 2) {
	$pc_tally[($r+7) % 12]+=$dur;
    }
    elsif($qual == 3) {
	$pc_tally[($r+8) % 12]+=$dur;
    }
    else {
	$pc_tally[($r+6) % 12]+=$dur;
    }


# If the symbol contains 7, 65, 43, or 42, it's a seventh chord. If the triad quality is major or aug: if the symbol 
# containsd, it's a dom seventh, otherwise it's a major seventh. If the quality is minor, it's a minor seventh. If the
# quality is diminished: if it contains x, it's fully-dim; if it contains h, it's half-dim.

    if($words[2] =~ /7|65|43|42/) {
	if($qual == 0 || $qual == 2) {
	    if($words[2] =~ /d/ || $words[3] == 7) {
		$pc_tally[($r+10) % 12]+=$dur;
	    }
	    else {
		$pc_tally[($r+11) % 12]+=$dur;
	    }
	}
	elsif($qual == 1) {
	    $pc_tally[($r+10) % 12]+=$dur;
	}
	elsif($qual == 3) {
	    if($words[2] =~ /x/) {
		$pc_tally[($r+9) % 12]+=$dur;
	    }
	    elsif($words[2] =~ /h/) {
		$pc_tally[($r+10) % 12]+=$dur;
	    }
	}
    }
}

$tally_total = 0;
for($pc=0; $pc<12; $pc++) {
    $tally_total += $pc_tally[$pc];
}

# print horizontally

if($v==1) {

    printf("Tally:\n");

    for($pc=0; $pc<12; $pc++) {
	printf("%6d ", $pc);
    }
    printf("\n");
    for($pc=0; $pc<12; $pc++) {
	printf("%6d ", $pc_tally[$pc]);
    }
    printf("\n");
    for($pc=0; $pc<12; $pc++) {
	printf("%6.3f ", $pc_tally[$pc] / $tally_total);
    }
    printf("\n");
}

elsif($v==0) {
    for($pc=0; $pc<12; $pc++) {
	$x = $pc_tally[$pc] / $tally_total;

	# To print out log P's instead of raw P's:
	# if($x==0.0) {
	#   $x = .001;
	# }
	# $x = log($x);

	printf("%.3f ", $x);
    }
    printf("\n");
}

elsif($v==-1) {
    for($pc=0; $pc<12; $pc++) {
	if($pc_tally[$pc] > 0.0) {
	    printf("1");
	}
	else {
	    printf("0");
	}
    }
    printf("\n");
}

@sd_names = ("1    ", "#1/b2", "2    ", "#2/b3", "3    ", "4    ", "#4/b5", "5    ", "#5/b6", "6    ", "#6/b7", "7    ");

# print vertically
# for($pc=0; $pc<12; $pc++) {
#  printf("%.7s %6d %6.3f\n", $sd_names[$pc], $pc_tally[$pc], $pc_tally[$pc] / $tally_total);
  # OR: printf("%6d (%5s) %6d %6.3f\n", $pc, $sd_names[$pc], $pc_tally[$pc], $pc_tally[$pc] / $tally_total);
#}
