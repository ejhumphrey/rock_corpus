
/*

This program takes a reduced harmonic analysis of a song, in which
repeated sections are represented with single symbols, and expands it
into one big long chord progression. See "comments" for more info.

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_ELEMENTS 1000   /* Maximum number of "elements" in a rule, where elements are strings on the left-hand side. "C*3" is two units. */
#define MAX_CHILDREN 1000   /* Maximum number of children in a rule; C*3 is three units. */

FILE *song_file;

int numwords;

/* Rewrite rules */
struct citem {
    char parent[10];
    char child[MAX_CHILDREN][10];
    int num_children;
    int current;           /* 1 if this symbol is currently being expanded, 0 otherwise */
    int used;              /* 1 if this symbol occurs in an expansion, 0 otherwise */
    struct citem * next;
};

struct citem * root;

struct {
    int type;             // 0 if a barline, 1 if it's a chord/rest symbol, 2 if it's a time sig, 3 if it's a key sig
    char string[10];
    int key;
    int timesig;          // (100 * numerator) + denominator, e.g. 2/4 = 204.
} raw_prog[1000];

int r;             /* Global variable for raw_prog elements as they are read in */
int num_raw_prog_elements;
int key, timesig;    /* Global variables for key and timesig as they are read in */

struct {
    int num_units;  /* Nmber of metrical units in harmonic analysis */
    int num_chords;  /* Number of metrical units in harmonic analysis */
    int chord[8];
    int timesig;
} measure[1000];

struct {
    char string[10];
    int key;
    int lroot;    /* int for roman numeral (or for first roman numeral in applied chord): 0=I, 1=bII, etc. */
    int sectonic;     /* int for second RN in applied chord (0=none, 7=V, etc.) */
    int croot;         /* int for chromatic root: croot = lroot + sectonic (mod 12) (0=1, 1=bII, etc.)  */
    int droot;         /* int for diatonic version of chromatic root: 0->1, 1->2, 2->2, 7->5, etc. */
    int aroot;         /* int for absolute root: key + croot (mod 12) */
    int pos;         /* Position in measure (first chord has position 0) */
    int measure;
    double start;      /* Timepoint at which chord starts in relation to measures, e.g. downbeat of m. 3 is 2.0 */
    double end;
} chord[1000];

struct {
    int key;
    int pos;
    int measure;
    double start;
} keysec[1000];

int num_measures, num_chords, num_keysecs;

int verbosity;       /* if 0; print chord list; if 1, print raw chord prog; if 2, print rules, raw prog, and chord list;
			if -1, print measure list with time sigs; if -2, print list of key sections */

read_analysis() {

    char line[1000];
    char pword[10];
    char inword[MAX_ELEMENTS][10];
    int isnumber[MAX_ELEMENTS+1];
    int i, j, w, num_children, num_elements, w2, num_occ, prev_child;
    char *s, *t;
    struct citem * tmp;

    root = NULL;

    while (fgets(line, sizeof(line), song_file) !=NULL) {
	for(i=0; i<MAX_ELEMENTS; i++) {
	    for(j=0; j<10; j++) inword[i][j]='\0';
	    isnumber[i] = 0;
	}
	isnumber[MAX_ELEMENTS] = 0;
	for(j=0; j<10; j++) pword[j]='\0';     /* pword is for first word of line (parent word) */

	/* Read in an input line. When you get to a space or a mid-string asterisk, start a new line. (No asterisk
	   at the beginning or end of a string. */

	//printf("%s ", line);
	for(s=line; *s!='\0'; s++) if(*s=='\t') *s=' ';
	s=line;

	w=-1;
	while(1) {

	    if(w > MAX_ELEMENTS) {
		printf("Error: Elements of rule '%s' exceed maximum number (%d)\n", pword, MAX_ELEMENTS);
		exit(1);
	    }

	    while(*s == ' ') s++;
	    if (*s == '\0' || *s == '\n' || *s == '%') break;
	    t = s;
	    while(*t != '\0' && *t != ' ' && *t != '\n' && *t != '*' && *t != '%' && *t != ':') t++;
	    if(*t==':') {
		if(w!=-1 || *(t+1)!=' ') {
		    printf("Error: Colon may only be used after parent word\n");
		    exit(1);
		}
		else t++;
	    }
	    if(t-s > 10) {
		printf("Error: String exceeds maximum length of 10 characters: %s\n", s);
		exit(1);
	    }
	    if(w==-1) strncpy(pword, s, t-s);
	    else strncpy(inword[w], s, t-s);
	    /* Now t-s is the length of inword[w] */

	    if(*t == '*') {        /* If an asterisk is encountered, the string is split into two. */
		if(w==-1) {
		    printf("Error: Asterisk may not be used on left-hand side of rule\n");
		    exit(1);
		}
		if(t==s) {
		    printf("Error: Asterisk must be preceded by string\n");
		    exit(1);
		}
		if(*s!='$' && *s!='|') {
		    printf("Error: Asterisk must be preceded by nonterminal\n");
		    exit(1);
		}
		isnumber[w+1] = 1; /* The following string SHOULD be a number - later we'll check if it really is */
		t++;
		if(*t == ' ' || *t == '\n') {
		    printf("Error: Asterisk must be followed by a number\n");
		    exit(1);
		}
	    }

	    if(w==-1) {
		if(pword[(t-s)-1] == ':') pword[(t-s)-1] = '\0';
		else {
		    printf("Error: No colon found after parent '%s'\n", pword);
		    exit(1);
		}
	    }
	    w++;
	    s = t;
	}

	if(w==-1) continue; /* Blank line or comment */
	if(w<1) {
	    printf("Error: Parent '%s' has no children\n", pword);
	    exit(1);
	}

	num_elements = w;

	/*
	printf("%s: ", pword);
	for(w=0; w<num_elements; w++) printf("%s ", inword[w]);
	printf("\n"); */

	s=pword;
	while(*s!='\0') {
	    if(*s=='$' || *s=='[' || *s==']') {
		printf("Error: Illegal character ('$', '[', or ']') on left-hand side of rule\n");
		exit(1);
	    }
	    s++;
	}

	if(strcmp(inword[0], ".")==0) {
	    printf("Error: '.' may not occur at beginning of expression\n");
	    exit(1);
	}
	for(w=0; w<num_elements; w++) {
	    if(inword[w][0] == '$' && *(inword[w]+1) == '\0') {
		printf("Error: '$' must be followed by string\n");
		exit(1);
	    }
	    if(inword[w][0] == '[' && *(inword[w]+1) == '\0') {
		printf("Error: '[' must be followed by string\n");
		exit(1);
	    }
	    if(inword[w][0] == ']') {
		printf("Error: ']' may only occur at end of string\n");
		exit(1);
	    }
	    s=inword[w]+1;
	    while(*s!='\0') {
		if(*s=='$') {
		    printf("Error: '$' may only occur at beginning of string\n");
		    exit(1);
		}
		if(*s=='[') {
		    printf("Error: '[' may only occur at beginning of string\n");
		    exit(1);
		}
		if(*s==']' && *(s+1)!='\0') {
		    printf("Error: ']' may only occur at end of string\n");
		    exit(1);
		}
		if(*(s+1)=='\0') {
		    if((inword[w][0]=='[' && *s!=']') || (inword[w][0]!='[' && *s==']')) {
			printf("Error: Unmatched '[' or ']'\n");
			exit(1);
		    }
		}
		s++;
	    }
	    if(isnumber[w]) {
		s=inword[w];
		while(*s!='\0') {
		    if(!(isdigit(*s))) {
			printf("Error: Asterisk must be followed by a number\n");
			exit(1);
		    }
		    s++;
		}
	    }
	}
	if(isnumber[num_elements]) {
	    printf("Error: Asterisk must be followed by a number\n");
	    exit(1);
	}

	/* create a new item */
	tmp = malloc(sizeof(struct citem));
	strcpy(tmp->parent, pword);
	tmp->current = 0;
	tmp->used = 0;
	w2=0;
	for(w=0; w<num_elements; w++) {
	    if(!(isnumber[w+1])) {
		if(w2==MAX_CHILDREN) {
		    printf("Error: Children of '%s' exceed maximum (%d)\n", tmp->parent, MAX_CHILDREN);
		    exit(1);
		}
		strcpy(tmp->child[w2], inword[w]);
		w2++;
	    }
	    else {
		sscanf(inword[w+1], "%d", &num_occ);
		for(i=0; i<num_occ; i++, w2++) {
		    if(w2==MAX_CHILDREN) {
			printf("Error: Children of '%s' exceed maximum (%d)\n", tmp->parent, MAX_CHILDREN);
			exit(1);
		    }
		    strcpy(tmp->child[w2], inword[w]);
		}
		w++;
	    }
	}

	tmp->num_children = w2;

	prev_child = -1;
	for(w=0; w<tmp->num_children; w++) {
	    if(strcmp(tmp->child[w], "|")==0) {
		if(prev_child == -1) {
		    printf("Error: Barline at beginning of definition of '%s'\n", tmp->parent);
		    exit(1);
		}
		else if(prev_child == 2) {
		    printf("Error: Barline following nonterminal '%s'\n", tmp->child[w-1]);
		    exit(1);
		}
		prev_child = 0;
	    }
	    if(tmp->child[w][0]!='|' && tmp->child[w][0]!='[' && tmp->child[w][0]!='$') {
		if(w==(tmp->num_children)-1) {
		    printf("Error: Terminal '%s' at end of expression must be followed by barline\n", tmp->child[w]);
		    exit(1);
		}
		prev_child = 1;
	    }
	    if(tmp->child[w][0]=='$') {
		if(prev_child == 1) {
		    printf("Error: Nonterminal '%s' is preceded by a terminal with no barline\n", tmp->child[w]);
		    exit(1);
		}
		prev_child = 2;
	    }
	}

	tmp->next = NULL;
	if(root != NULL) tmp->next = root;
	root = tmp;

	/* Make sure it hasn't already been defined */
	tmp = root;
	while(1) {
	    tmp = tmp->next;
	    if(tmp == NULL) break;
	    if(strcmp(tmp->parent, root->parent)==0) {
		printf("Error: Multiple definitions found for '%s'\n", tmp->parent);
		exit(1);
	    }
	}
    }
}

output_all_rules() {

    int i, c2;
    struct citem * tmp;

    tmp = root;
    printf("\nRULES:\n");
    while(1) {
	if(tmp == NULL) break;

	printf("%s: ", tmp->parent);
	printf("(%d) ", tmp->num_children);
	for(c2=0; c2<tmp->num_children; c2++) {
	    printf("%s ", tmp->child[c2]);
	}
	printf("\n");
	tmp = tmp->next;
    }
    printf("\n");
}

read_key_or_timesig(char * s) {

    char * t;
    char s2[10];
    char * s3;
    char tsstring[2];
    int k, bad_ts, ti, len;

    t=s+1;
    while(*t != ']') t++;
    strncpy(s2, s+1, t-(s+1));
    s2[t-(s+1)]='\0';

    //printf("Key/timesig symbol: %s\n", s2);

    k = -1;
    if(strcmp(s2, "C")==0) k=0;
    else if(strcmp(s2, "C#")==0 || strcmp(s2, "Db")==0) k=1;
    else if(strcmp(s2, "D")==0) k=2;
    else if(strcmp(s2, "Eb")==0 || strcmp(s2, "D#")==0) k=3;
    else if(strcmp(s2, "E")==0) k=4;
    else if(strcmp(s2, "F")==0) k=5;
    else if(strcmp(s2, "F#")==0 || strcmp(s2, "Gb")==0) k=6;
    else if(strcmp(s2, "G")==0) k=7;
    else if(strcmp(s2, "Ab")==0 || strcmp(s2, "G#")==0) k=8;
    else if(strcmp(s2, "A")==0) k=9;
    else if(strcmp(s2, "Bb")==0 || strcmp(s2, "A#")==0) k=10;
    else if(strcmp(s2, "B")==0) k=11;

    if(k!=-1) {
	key = k;
	raw_prog[r].key = key;
	return 3;
    }

    // If we got here, it's not a valid key signature; assume it's a time signature

    bad_ts = 0;
    s3 = s2;
    if(strcmp(s2, "0")==0) {
	timesig = 0;
	return 2;
    }
    else if(*(s3+1) == '/') len = 1;
    else if(*(s3+2) == '/') len = 2;

    else bad_ts = 1;
    if(bad_ts == 0) {

	//printf("The char is %c\n", *s3);
	strncpy(tsstring, s3, len);
	tsstring[len] = '\0';
	//tsstring[1] = '\0';
	sscanf(tsstring, "%d", &ti);
	timesig = 100 * ti;
	//printf("Now timesig is %d\n", timesig);
	s3 += (len+1);

	if(*(s3+1) == '\0') len = 1;
	else if(*(s3+2) == '\0') len = 2;
	else bad_ts = 1;
	if(bad_ts == 0) {
	    strncpy(tsstring, s3, len);
	    tsstring[len] = '\0';
	    sscanf(tsstring, "%d", &ti);
	    timesig += ti;
	}
    }

    // Check for timesigs with bad nums or denoms?
    //printf("timesig = %d\n", timesig);

    if(bad_ts == 0) return 2;

    else {
	printf("Key or time signature string '%s' not recognized\n", s2);
	exit(1);
    }

}

expand(char * parent) {

    /* We expand the hierarchical representation into a "raw chord progression". This is a series of chords and barlines. Key and time
       signature symbols are removed, and the key and meter info is added to chords and barlines, respectively. */

    int i, nonterminal, x, r2;
    int local_key, local_timesig;
    struct citem * ci;
    char * adj_parent;

    if(parent[0] == '[') {
	raw_prog[r].type = read_key_or_timesig(parent);    /* x=2 if it's a timesig, x=3 if it's a keysig */
	if(raw_prog[r].type==2) {
	    /* We've found a time signature. Work backwards from it until you hit a barline. If you hit a chord symbol
	       first, it's an error. */
	    for(r2=r-1; r2 >= 0; r2--) {
		if(raw_prog[r2].type == 0 || raw_prog[r2].type == 2) break;
		else if(raw_prog[r2].type == 3) continue;
		printf("Error: Time signature must be at the beginning of a measure\n");
		exit(1);
	    }
	}
	r++;
	return 0;
    }
    else if(parent[0] == '$') {
	nonterminal = 1;
	adj_parent = parent+1;
    }
    else {
	nonterminal = 0;
	adj_parent = parent;
    }

    /* Search the list for a match */
    ci = root;
    while(1) {
	if(ci == NULL) break;
	if(strcmp(ci->parent, adj_parent)==0) {
	    /* Found a match */
	    if(!(nonterminal)) {
		printf("\nError: '%s' is marked as terminal (no $) but is defined\n", parent);
		exit(1);
	    }
	    ci->used = 1;
	    if(ci->current==1) {
		printf("\nError: '%s' occurs in the expansion of '%s' (infinite recursion)\n", parent, adj_parent);
		exit(1);
	    }
	    ci->current = 1;
	    //printf("Found %s, num children = %d\n", adj_parent, ci->num_children);
	    for(i=0; i<ci->num_children; i++) {
		if(key==-1 && i>0) {
		    printf("Error: Key must be specified at beginning of 'S' definition\n");
		    exit(1);
		}
		local_key = key;
		local_timesig = timesig;
		expand(ci->child[i]);
		/* If the child is a nonterminal, you need to reset the key and timesig; otherwise it won't have been
		   changed, so there's no need. */
		if(ci->child[i][0]=='$') {
		    timesig = local_timesig;
		    //key = local_key;
		    if(local_key != key) {
			key = local_key;
			raw_prog[r].type = 3;
			raw_prog[r].key = key;
			r++;
		    }
		}
	    }
	    ci->current = 0;
	    return 1;
	}
	ci = ci->next;
    }

    /* No match was found; it should be a terminal */
    if(strcmp(adj_parent, "S")==0) {
	printf("Error: No definition found for top-level symbol 'S'\n");
	exit(1);
    }
    if(nonterminal) {
	printf("\nError: No definition found for non-terminal '%s'\n", parent);
	exit(1);
    }
    //printf("%s(%d/%d) ", parent, key, timesig);
    //printf("%s ", parent);
    if(strcmp(parent, "|")==0) {
	/* Here we assign a time signature to each barline. This is not quite correct since the barline BEFORE the time signature
	   symbol doesn't get it. But we fix this later, see main(). */
	raw_prog[r].type = 0;
	raw_prog[r].timesig = timesig;
    }
    else {
	raw_prog[r].type = 1;
	strcpy(raw_prog[r].string, parent);
	raw_prog[r].key = key;
    }
    r++;
}

print_key(int k) {
    //printf("Here!\n");
    if(k==0) printf("[C] ");
    else if(k==1) printf("[Db] ");
    else if(k==2) printf("[D] ");
    else if(k==3) printf("[Eb] ");
    else if(k==4) printf("[E] ");
    else if(k==5) printf("[F] ");
    else if(k==6) printf("[F#] ");
    else if(k==7) printf("[G] ");
    else if(k==8) printf("[Ab] ");
    else if(k==9) printf("[A] ");
    else if(k==10) printf("[Bb] ");
    else if(k==11) printf("[B] ");
}

print_raw_prog() {

    int prev_key, prev_timesig;
    int t, timesig_num, timesig_denom;
    char * prev_chord;
    char x = 'x';
    prev_chord = &x;  /* Initialize it to something so we can do strcmp later */

    prev_key = -1;
    prev_timesig = raw_prog[0].timesig;

    for(r=1; r<num_raw_prog_elements; r++) {
	if(raw_prog[r].type==1) {
	    if(raw_prog[r].key != prev_key) {
		print_key(raw_prog[r].key);
	    }
	    if((strcmp(prev_chord, raw_prog[r].string)!=0 && strcmp(raw_prog[r].string, ".")!=0) || raw_prog[r].key != prev_key) {
		printf("%s ", raw_prog[r].string);
		prev_chord = raw_prog[r].string;
	    }
	    else if(r<num_raw_prog_elements-1) {
		if(!(raw_prog[r-1].type==0 && raw_prog[r+1].type==0)) {
		    printf(". ");
		}
	    }
	    prev_key = raw_prog[r].key;
	}
	else if(raw_prog[r].type==0) {
	    printf("| ");
	    if(raw_prog[r].timesig != prev_timesig) {
		t = raw_prog[r].timesig;
		if(t==0) printf("[0] ");
		else {
		    timesig_num = (t - (t % 100)) / 100;
		    timesig_denom = t % 100;
		    printf("[%d/%d] ", timesig_num, timesig_denom);
		}
		prev_timesig = raw_prog[r].timesig;
	    }
	}
    }
    printf("\n");
    if(verbosity > 1) {
	printf("\n");
    }
}

create_chords_and_measures() {

    int m, c, n, p, c2, current_timesig, ks;

    c = num_measures = num_chords = num_keysecs = ks = 0;
    m = -1;       /* Remember the first element of the raw progression is an (invisible) barline. As soon as a real element is encountered,
		     this gets incremented to zero. */

    /* Rests (R) are absorbed into the previous chord. If a rest occurs at the
       beginning of the song, the first chord starts after it. */

    current_timesig = raw_prog[0].timesig;

    for(r=0; r<num_raw_prog_elements; r++) {
	if(raw_prog[r].type == 0) {
	    if(m>-1) {
		measure[m].num_chords = n;
		measure[m].num_units = p;
		measure[m].timesig = current_timesig;    /* Assign the just-ended measure the timesig of the PREVIOUS barline */
		current_timesig = raw_prog[r].timesig;
	    }
	    m++;
	    n=p=0;
	}
	else if(raw_prog[r].type == 1) {                /* It's a chord */
	    if(c>0) {
		if((strcmp(raw_prog[r].string, chord[c-1].string)==0 && raw_prog[r].key==chord[c-1].key) || strcmp(raw_prog[r].string, ".")==0) {
		    p++;
		    continue;
		}
	    }
	    if(strcmp(raw_prog[r].string, "R")==0) {
		p++;
		continue;
	    }
	    strcpy(chord[c].string, raw_prog[r].string);
	    chord[c].key = raw_prog[r].key;
	    chord[c].pos = p;
	    chord[c].measure = m;
	    if(n<7) measure[m].chord[n] = c;
	    /* ^ Can't have more than 8 chords per measure */
	    p++;
	    n++;
	    c++;
	}
	else if(raw_prog[r].type == 3) {                /* It's a key symbol */
	    if(ks > 0) {
		if(keysec[ks-1].pos == p && keysec[ks-1].measure == m) ks--;
		if(raw_prog[r].key == keysec[ks-1].key) continue;
	    }
	    keysec[ks].key = raw_prog[r].key;
	    keysec[ks].pos = p;
	    keysec[ks].measure = m;
	    ks++;
	}
    }

    num_measures = m;
    num_chords = c;
    num_keysecs = ks;

    for(c=0; c<num_chords; c++) {
	m = chord[c].measure;
	chord[c].start = (double)(m) + ((double)(chord[c].pos) / (double)(measure[m].num_units));
    }
    for(ks=0; ks<num_keysecs; ks++) {
	m = keysec[ks].measure;
	if(m==num_measures) {
	    num_keysecs--;
	    break;
	}
	keysec[ks].start = (double)(m) + ((double)(keysec[ks].pos) / (double)(measure[m].num_units));
    }

    if(verbosity > 1) {
	printf("Chords: ");
	for(c=0; c<num_chords; c++) {
	    printf("%s (%5.3f) ", chord[c].string, chord[c].start);
	}
	printf("\n\n");
    }

    if(verbosity == -1) {
	for(m=0; m<num_measures; m++) {
	    printf("%d %d\n", m+1, measure[m].timesig);
	}
    }

    if(verbosity == -2) {
	for(ks=0; ks<num_keysecs; ks++) {
	    printf("%.3f %d\n", keysec[ks].start, keysec[ks].key);
	}
    }

}

romnum(char * cstring) {

    char rn[4];
    char * s;

    //if(strcmp(cstring, "R")==0) return -1;

    s=cstring;

    if(*s == 'b' || *s == '#') s++;

    while(*s=='I' || *s=='i' || *s=='V' || *s=='v') s++;

    if(s-cstring > 4 || s==cstring) {

	printf("Roman numeral '%s' not recognized\n", cstring);
	exit(1);
    }

    strncpy(rn, cstring, s-cstring);
    rn[s-cstring]='\0';

    if(strcmp(rn, "I")==0 || strcmp(rn, "i")==0) return 0;
    else if(strcmp(rn, "#I")==0 || strcmp(rn, "#i")==0) return 1;
    else if(strcmp(rn, "bII")==0 || strcmp(rn, "bii")==0) return 1;
    else if(strcmp(rn, "II")==0 || strcmp(rn, "ii")==0) return 2;
    else if(strcmp(rn, "#II")==0 || strcmp(rn, "#ii")==0) return 3;
    else if(strcmp(rn, "bIII")==0 || strcmp(rn, "biii")==0) return 3;
    else if(strcmp(rn, "III")==0 || strcmp(rn, "iii")==0) return 4;
    else if(strcmp(rn, "IV")==0 || strcmp(rn, "iv")==0) return 5;
    else if(strcmp(rn, "#IV")==0 || strcmp(rn, "#iv")==0) return 6;
    else if(strcmp(rn, "bV")==0 || strcmp(rn, "bv")==0) return 6;
    else if(strcmp(rn, "V")==0 || strcmp(rn, "v")==0) return 7;
    else if(strcmp(rn, "#V")==0 || strcmp(rn, "#v")==0) return 8;
    else if(strcmp(rn, "bVI")==0 || strcmp(rn, "bvi")==0) return 8;
    else if(strcmp(rn, "VI")==0 || strcmp(rn, "vi")==0) return 9;
    else if(strcmp(rn, "#VI")==0 || strcmp(rn, "#vi")==0) return 10;
    else if(strcmp(rn, "bVII")==0 || strcmp(rn, "bvii")==0) return 10;
    else if(strcmp(rn, "VII")==0 || strcmp(rn, "vii")==0) return 11;
    else {
	printf("Roman numeral '%s' not recognized\n", rn);
	exit(1);
    }

}

chr_to_dia(int x) {

    if(x==0) return 1;
    else if(x<3) return 2;
    else if(x<5) return 3;
    else if(x<7) return 4;
    else if(x<8) return 5;
    else if(x<10) return 6;
    else return 7;

}

assign_roots() {

    int c, root;
    char * s;
    for(c=0; c<num_chords; c++) {

	chord[c].lroot = romnum(chord[c].string);
	chord[c].sectonic = 0;
	s = chord[c].string;

	while(*s != '\0' && *s != '/') s++;
	/* Now s applies to the character after the roman numeral */
	if(*s == '/') {
	    s++;
	    chord[c].sectonic = romnum(s);
	}

	chord[c].croot = (chord[c].lroot + chord[c].sectonic) % 12;
	chord[c].droot = chr_to_dia(chord[c].croot);
	chord[c].aroot = (chord[c].croot + chord[c].key) % 12;

    }

}

main(int argc, char * argv[]) {

    int a, r2, c, c2, last_chord;
    struct citem * ci;

    verbosity=1;

    song_file = stdin;
    for(a=1; a < argc; a++) {
	if(strcmp(argv[a], "-v")==0) {
	    sscanf(argv[a+1], "%d", &verbosity);
	    a++;
	}
	else {
	    song_file = fopen(argv[a], "r");
	    if(song_file == NULL) {
		printf("Input file '%s' not found\n", argv[a]);
		exit(1);
	    }
	}
    }

    read_analysis();

    if(verbosity > 1) output_all_rules();

    num_raw_prog_elements = 0;

    key=-1;
    timesig=404;   /* Assume 4/4 as default */
    raw_prog[0].type = 0;   /* Assume a barline at the beginning of the piece */
    raw_prog[0].timesig = 404;
    r=1;
    expand("$S");

    num_raw_prog_elements = r;

    if(verbosity > 0) {
	ci = root;
	while(1) {
	    if(ci == NULL) break;
	    if(ci->used == 0) {
		printf("Warning: '%s' is defined but never used\n", ci->parent);
	    }
	    ci=ci->next;
	}
    }

    /* Go through the barlines left to right. Assign each barline the time signature of the FOLLOWING barline. */
    for(r=0; r<num_raw_prog_elements; r++) {
	if(raw_prog[r].type == 0) {
	    for(r2=r+1; r2<num_raw_prog_elements; r2++) {
		if(raw_prog[r2].type == 0) {
		    raw_prog[r].timesig = raw_prog[r2].timesig;
		    break;
		}
	    }
	}
    }

    if(verbosity >= 1) {
	if(verbosity > 1) printf("Raw chord progression:\n");
	print_raw_prog();
    }

    create_chords_and_measures();

    assign_roots();

    c=0;
    while(c<(num_chords-1)) {
	/* Set the end time for each chord c to the start time of chord c2=c+1 */

	c2 = c+1;
	/* If c2 has the same root and key as c, then c2 is not a NEW chord and no chord statement will be printed for it.
	   Increment c2 until you get a new chord; if c2 is the last chord, stop */
	while(c2<num_chords && chord[c2].croot == chord[c].croot && chord[c2].key == chord[c].key) {
	    c2++;
	}
	if(c2 == num_chords) break;
	chord[c].end = chord[c2].start;
	/* Now c2 is the next NEW chord */
	c = c2;
    }
    /* Now set the end time for the last NEW chord. If c2 = num_chords, that means the last new chord is c. Otherwise, the last new
       chord is the final chord. */
    chord[c].end = (double)(num_measures);
    //else chord[c].end = (double)(num_measures);

    if(verbosity > 1) printf("Chords w/ timepoints and roots:\n");

    if(verbosity == 0 || verbosity > 1) {
	for(c=0; c<num_chords; c++) {
	    if(c > 0) if(chord[c].croot == chord[c-1].croot && chord[c].key == chord[c-1].key) continue;
	    /* ^ This might occur if one chord c is an applied chord, or if they're different inversions. In this case, chord c has already
	       been subsumed by the previous chord (the previous chord's end time has been set to c's end time ) */
	    printf("%5.2f %5.2f %5s %3d %3d %3d %3d\n", chord[c].start, chord[c].end, chord[c].string, chord[c].croot, chord[c].droot, chord[c].key, chord[c].aroot);
	}
	printf("---\n");
	if(verbosity>1) printf("\n");
    }

}

