#!/usr/bin/perl

use strict;
use warnings;


my @connections;
open(my $in,  "<",  $ARGV[0])  or die "Can't open file: $!";
open(my $fhc, '>', "$ARGV[1].c") or die "Can't open file: $!";
open(my $fhh, '>', "$ARGV[1].h") or die "Can't open file: $!";
print $fhh "//THIS FILE IS GENERATED!\n//DO NOT TOUCH!!!\n\n";
print $fhh "#ifndef ___$ARGV[1]_H\n";
print $fhh "#define ___$ARGV[1]_H\n";
print $fhh "#include <avr/io.h>\n";
while (<$in>){
  if(substr($_,0,2) ne "//"){
    if($_ =~ /^(\S*)\s+(\S)(\d)\s+(\S)\s+(\S*)/){
      my $name = $1;
      my $port = $2;
      my $pin = $3;
      my $dir = $4;
      my $negate = $5;
      my @con = ($1, $2, $3, $4, $5);
      #push @connections, [@con];
      #print "name = $1\n";
      #print "port = $2\n";
      #print "pin = $3\n";
      #print "direction = $dir\n";
      if($dir eq "i"){
        print $fhh "// $name (input)\n";
      }else{
        print $fhh "// $name (output)\n";
      }
      print $fhh "#define $name P$port$pin\n";
      print $fhh "#define ${name}_PORT PORT$port\n";
      print $fhh "#define ${name}_DDR DDR$port\n";
      print $fhh "#define ${name}_PIN PIN$port\n";
      if($dir eq "i"){
        if($negate eq "n"){
          print $fhh "#define ${name}_IS_ON (!(${name}_PIN & (1<<P$port$pin)))\n";
        }else{
          print $fhh "#define ${name}_IS_ON (${name}_PIN & (1<<P$port$pin))\n";
        }
        print $fhh "#define ${name}_SET_PULLUP (${name}_PORT |= (1<<P$port$pin))\n";
        print $fhh "#define ${name}_CLEAR_PULLUP (${name}_PORT &= ~(1<<P$port$pin))\n";
      }else{  
        if($negate eq "n"){
          print $fhh "#define ${name}_ON (${name}_PORT &= ~(1<<P$port$pin))\n";
          print $fhh "#define ${name}_OFF (${name}_PORT |= (1<<P$port$pin))\n";
        }else{
          print $fhh "#define ${name}_ON (${name}_PORT |= (1<<P$port$pin))\n";
          print $fhh "#define ${name}_OFF (${name}_PORT &= ~(1<<P$port$pin))\n";
        }
        print $fhh "#define ${name}_TOGGLE (${name}_PORT ^= (1<<P$port$pin))\n";
              
      }
      print $fhh "\n";
    }
  }

}
print $fhh "\nvoid $ARGV[1]_init(void);\n";
print $fhh "#endif\n";
open(my $in2,  "<",  $ARGV[0])  or die "Can't open file: $!";
print $fhc "//THIS FILE IS GENERATED!\n//DO NOT TOUCH!!!\n\n";
print $fhc "#include \"$ARGV[1].h\"\n";
print $fhc "\nvoid $ARGV[1]_init(){\n";
while (<$in2>){
  #print "$_";
  if(substr($_,0,2) ne "//"){
    if($_ =~ /^(\S*)\s+(\S)(\d)\s+(\S)/){
      #print "match\n";
      my $name = $1;
      my $port = $2;
      my $pin = $3;
      my $dir = $4;
      my @con = ($1, $2, $3, $4);
      if($dir eq "i"){
        print $fhc "  ${name}_DDR &= ~(1<<P$port$pin);\n";
      }else{
        print $fhc "  ${name}_DDR |= (1<<P$port$pin);\n";
        print $fhc "  ${name}_OFF;\n";
      }
    }
  }

}
print $fhc "}\n";

close($fhh);
close($fhc);
