#!/usr/bin/perl

$update = "./updateGGP.pl";
$runSTP = "./runSourceTargetPair.pl";
$runSSTP = "./runSourceSourceTargetPair.pl";

$env = $ARGV[0];
$level = $ARGV[1];
$scenario = $ARGV[2];

$stats = "./runStats.pl";

if ($env =~ "10") {
  # level 10 has no environment
  $scenario = $level;
  $level = 10;
  die unless ($#ARGV == 1);
}
else {
  die unless  ($#ARGV == 2);
}

if ($env =~ /^e/) {
  if ($level == 6) {
    print `$stats escape-tl$level-$scenario-source1.log`;
    print "\n";
    print `$stats escape-tl$level-$scenario-source2.log`;
  }
  else {
    print `$stats escape-tl$level-$scenario-source.log`;
  }
  print "\n";
  print `$stats escape-tl$level-$scenario-target_no_source.log`;
  print "\n";
  print `$stats escape-tl$level-$scenario-target_after_source.log`;
}
elsif ($env =~ /^w/ or $env =~ /^m/) {
  if ($level == 6) {
    print `$stats wargame-tl$level-$scenario-source1.log`;
    print "\n";
    print `$stats wargame-tl$level-$scenario-source2.log`;
  }
  else {
    print `$stats wargame-tl$level-$scenario-source.log`;
  }
  print "\n";
  print `$stats wargame-tl$level-$scenario-target_no_source.log`;
  print "\n";
  print `$stats wargame-tl$level-$scenario-target_after_source.log`;
}
elsif ($env =~ /^r/) {
  $env = "rogue";
  if ($level == 6) {
    print `$stats mRogue-TL-Level-$level-$scenario-Source1.log`;
    print "\n";
    print `$stats mRogue-TL-Level-$level-$scenario-Source2.log`;
  }
  else {
    print `$stats mRogue-TL-Level-$level-$scenario-Source.log`;
  }
  print "\n";
  print `$stats mRogue-TL-Level-$level-$scenario-Target_no_source.log`;
  print "\n";
  print `$stats mRogue-TL-Level-$level-$scenario-Target_after_source.log`;
}
elsif ($level =~ "10") {
  print `$runStats TL-Level-10-$scenario-Source.log`;
  print "\n";
  print `$runStats TL-Level-10-$scenario-Target_no_source.log`;
  print "\n";
  print `$runStats TL-Level-10-$scenario-Target_after_source.log`;
}
else {die "bad environment";}

