#!/usr/bin/env perl

# ====================================================================
# [Re]written by Andy Polyakov <appro@fy.chalmers.se> for the OpenSSL
# project. The module is, however, dual licensed under OpenSSL and
# CRYPTOGAMS licenses depending on where you obtain it. For further
# details see http://www.openssl.org/~appro/cryptogams/.
# ====================================================================

# At some point it became apparent that the original SSLeay ARC4
# assembler implementation performs suboptimally on latest IA-32
# microarchitectures. After re-tuning performance has changed as
# following:
#
# Pentium	-10%
# Pentium III	+12%
# AMD		+50%(*)
# P4		+250%(**)
#
# (*)	This number is actually a trade-off:-) It's possible to
#	achieve	+72%, but at the cost of -48% off PIII performance.
#	In other words code performing further 13% faster on AMD
#	would perform almost 2 times slower on Intel PIII...
#	For reference! This code delivers ~80% of arc4-amd64.pl
#	performance on the same Opteron machine.
# (**)	This number requires compressed key schedule set up by
#	ARC4_set_key [see commentary below for further details].
#
#					<appro@fy.chalmers.se>

$0 =~ m/(.*[\/\\])[^\/\\]+$/; $dir=$1;
push(@INC,"${dir}","${dir}../../perlasm");
require "x86asm.pl";

&asm_init($ARGV[0],"arc4-586.pl");

$xx="eax";
$yy="ebx";
$tx="ecx";
$ty="edx";
$inp="esi";
$out="ebp";
$dat="edi";

sub ARC4_loop {
  my $i=shift;
  my $func = ($i==0)?*mov:*or;

	&add	(&LB($yy),&LB($tx));
	&mov	($ty,&DWP(0,$dat,$yy,4));
	&mov	(&DWP(0,$dat,$yy,4),$tx);
	&mov	(&DWP(0,$dat,$xx,4),$ty);
	&add	($ty,$tx);
	&inc	(&LB($xx));
	&and	($ty,0xff);
	&ror	($out,8)	if ($i!=0);
	if ($i<3) {
	  &mov	($tx,&DWP(0,$dat,$xx,4));
	} else {
	  &mov	($tx,&wparam(3));	# reload [re-biased] out
	}
	&$func	($out,&DWP(0,$dat,$ty,4));
}

# void ARC4(ARC4_KEY *key,size_t len,const unsigned char *inp,unsigned char *out);
&function_begin("ARC4");
	&mov	($dat,&wparam(0));	# load key schedule pointer
	&mov	($ty, &wparam(1));	# load len
	&mov	($inp,&wparam(2));	# load inp
	&mov	($out,&wparam(3));	# load out

	&xor	($xx,$xx);		# avoid partial register stalls
	&xor	($yy,$yy);

	&cmp	($ty,0);		# safety net
	&je	(&label("abort"));

	&mov	(&LB($xx),&BP(0,$dat));	# load key->x
	&mov	(&LB($yy),&BP(4,$dat));	# load key->y
	&add	($dat,8);

	&lea	($tx,&DWP(0,$inp,$ty));
	&sub	($out,$inp);		# re-bias out
	&mov	(&wparam(1),$tx);	# save input+len

	&inc	(&LB($xx));

	# detect compressed key schedule...
	&cmp	(&DWP(256,$dat),-1);
	&je	(&label("ARC4_CHAR"));

	&mov	($tx,&DWP(0,$dat,$xx,4));

	&and	($ty,-4);		# how many 4-byte chunks?
	&jz	(&label("loop1"));

	&lea	($ty,&DWP(-4,$inp,$ty));
	&mov	(&wparam(2),$ty);	# save input+(len/4)*4-4
	&mov	(&wparam(3),$out);	# $out as accumulator in this loop

	&set_label("loop4",16);
		for ($i=0;$i<4;$i++) { ARC4_loop($i); }
		&ror	($out,8);
		&xor	($out,&DWP(0,$inp));
		&cmp	($inp,&wparam(2));	# compare to input+(len/4)*4-4
		&mov	(&DWP(0,$tx,$inp),$out);# $tx holds re-biased out here
		&lea	($inp,&DWP(4,$inp));
		&mov	($tx,&DWP(0,$dat,$xx,4));
	&jb	(&label("loop4"));

	&cmp	($inp,&wparam(1));	# compare to input+len
	&je	(&label("done"));
	&mov	($out,&wparam(3));	# restore $out

	&set_label("loop1",16);
		&add	(&LB($yy),&LB($tx));
		&mov	($ty,&DWP(0,$dat,$yy,4));
		&mov	(&DWP(0,$dat,$yy,4),$tx);
		&mov	(&DWP(0,$dat,$xx,4),$ty);
		&add	($ty,$tx);
		&inc	(&LB($xx));
		&and	($ty,0xff);
		&mov	($ty,&DWP(0,$dat,$ty,4));
		&xor	(&LB($ty),&BP(0,$inp));
		&lea	($inp,&DWP(1,$inp));
		&mov	($tx,&DWP(0,$dat,$xx,4));
		&cmp	($inp,&wparam(1));	# compare to input+len
		&mov	(&BP(-1,$out,$inp),&LB($ty));
	&jb	(&label("loop1"));

	&jmp	(&label("done"));

# this is essentially Intel P4 specific codepath...
&set_label("ARC4_CHAR",16);
	&movz	($tx,&BP(0,$dat,$xx));
	# strangely enough unrolled loop performs over 20% slower...
	&set_label("cloop1");
		&add	(&LB($yy),&LB($tx));
		&movz	($ty,&BP(0,$dat,$yy));
		&mov	(&BP(0,$dat,$yy),&LB($tx));
		&mov	(&BP(0,$dat,$xx),&LB($ty));
		&add	(&LB($ty),&LB($tx));
		&movz	($ty,&BP(0,$dat,$ty));
		&add	(&LB($xx),1);
		&xor	(&LB($ty),&BP(0,$inp));
		&lea	($inp,&DWP(1,$inp));
		&movz	($tx,&BP(0,$dat,$xx));
		&cmp	($inp,&wparam(1));
		&mov	(&BP(-1,$out,$inp),&LB($ty));
	&jb	(&label("cloop1"));

&set_label("done");
	&dec	(&LB($xx));
	&mov	(&BP(-4,$dat),&LB($yy));	# save key->y
	&mov	(&BP(-8,$dat),&LB($xx));	# save key->x
&set_label("abort");
&function_end("ARC4");

########################################################################

$inp="esi";
$out="edi";
$idi="ebp";
$ido="ecx";
$idx="edx";

&external_label("OPENSSL_ia32cap_P");

# void ARC4_set_key(ARC4_KEY *key,int len,const unsigned char *data);
&function_begin("ARC4_set_key");
	&mov	($out,&wparam(0));		# load key
	&mov	($idi,&wparam(1));		# load len
	&mov	($inp,&wparam(2));		# load data
	&picmeup($idx,"OPENSSL_ia32cap_P");

	&lea	($out,&DWP(2*4,$out));		# &key->data
	&lea	($inp,&DWP(0,$inp,$idi));	# $inp to point at the end
	&neg	($idi);
	&xor	("eax","eax");
	&mov	(&DWP(-4,$out),$idi);		# borrow key->y

	&bt	(&DWP(0,$idx),20);		# check for bit#20
	&jc	(&label("c1stloop"));

&set_label("w1stloop",16);
	&mov	(&DWP(0,$out,"eax",4),"eax");	# key->data[i]=i;
	&add	(&LB("eax"),1);			# i++;
	&jnc	(&label("w1stloop"));

	&xor	($ido,$ido);
	&xor	($idx,$idx);

&set_label("w2ndloop",16);
	&mov	("eax",&DWP(0,$out,$ido,4));
	&add	(&LB($idx),&BP(0,$inp,$idi));
	&add	(&LB($idx),&LB("eax"));
	&add	($idi,1);
	&mov	("ebx",&DWP(0,$out,$idx,4));
	&jnz	(&label("wnowrap"));
	  &mov	($idi,&DWP(-4,$out));
	&set_label("wnowrap");
	&mov	(&DWP(0,$out,$idx,4),"eax");
	&mov	(&DWP(0,$out,$ido,4),"ebx");
	&add	(&LB($ido),1);
	&jnc	(&label("w2ndloop"));
&jmp	(&label("exit"));

# Unlike all other x86 [and x86_64] implementations, Intel P4 core
# [including EM64T] was found to perform poorly with above "32-bit" key
# schedule, a.k.a. ARC4_INT. Performance improvement for IA-32 hand-coded
# assembler turned out to be 3.5x if re-coded for compressed 8-bit one,
# a.k.a. ARC4_CHAR! It's however inappropriate to just switch to 8-bit
# schedule for x86[_64], because non-P4 implementations suffer from
# significant performance losses then, e.g. PIII exhibits >2x
# deterioration, and so does Opteron. In order to assure optimal
# all-round performance, we detect P4 at run-time and set up compressed
# key schedule, which is recognized by ARC4 procedure.

&set_label("c1stloop",16);
	&mov	(&BP(0,$out,"eax"),&LB("eax"));	# key->data[i]=i;
	&add	(&LB("eax"),1);			# i++;
	&jnc	(&label("c1stloop"));

	&xor	($ido,$ido);
	&xor	($idx,$idx);
	&xor	("ebx","ebx");

&set_label("c2ndloop",16);
	&mov	(&LB("eax"),&BP(0,$out,$ido));
	&add	(&LB($idx),&BP(0,$inp,$idi));
	&add	(&LB($idx),&LB("eax"));
	&add	($idi,1);
	&mov	(&LB("ebx"),&BP(0,$out,$idx));
	&jnz	(&label("cnowrap"));
	  &mov	($idi,&DWP(-4,$out));
	&set_label("cnowrap");
	&mov	(&BP(0,$out,$idx),&LB("eax"));
	&mov	(&BP(0,$out,$ido),&LB("ebx"));
	&add	(&LB($ido),1);
	&jnc	(&label("c2ndloop"));

	&mov	(&DWP(256,$out),-1);		# mark schedule as compressed

&set_label("exit");
	&xor	("eax","eax");
	&mov	(&DWP(-8,$out),"eax");		# key->x=0;
	&mov	(&DWP(-4,$out),"eax");		# key->y=0;
&function_end("ARC4_set_key");

# const char *ARC4_options(void);
&function_begin_B("ARC4_options");
	&call	(&label("pic_point"));
&set_label("pic_point");
	&blindpop("eax");
	&lea	("eax",&DWP(&label("opts")."-".&label("pic_point"),"eax"));
	&picmeup("edx","OPENSSL_ia32cap_P");
	&bt	(&DWP(0,"edx"),20);
	&jnc	(&label("skip"));
	  &add	("eax",12);
	&set_label("skip");
	&ret	();
&set_label("opts",64);
&asciz	("arc4(4x,int)");
&asciz	("arc4(1x,char)");
&asciz	("ARC4 for x86, CRYPTOGAMS by <appro\@openssl.org>");
&align	(64);
&function_end_B("ARC4_options");

&asm_finish();

