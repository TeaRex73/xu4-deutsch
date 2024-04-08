#!/usr/bin/perl
use strict;
use warnings;
use Digest::CRC qw(crc32);
use IO::Compress::Deflate qw(deflate $DeflateError :constants) ;

# Our beloved fixed palette
my @palette=(
      [  0,  0,  0],
      [ 54,146,255],
      [ 60,204,  0],
      [241,241,241],
      [214, 67,255],
      [216,115,  0],
   );

################################################################################
# Take chunk of PNG data as parameter, calculate its length & CRC, and output it
################################################################################
sub PNGoutputChunk()
{
   my $len=length($_[0])-4;
   my $crc = Digest::CRC->new(type=>"crc32");
   $crc->add($_[0]);
   print pack('N',$len),$_[0],pack('N',$crc->digest);
}

################################################################################
# Main
################################################################################

   # Read P6 PNM file from STDIN
   my $line = <STDIN>;
   chomp($line);
   if ($line ne "P6"){die "Expected P6 format PNM file"}

   # Read width and height from STDIN
   $line = <STDIN>;
   my ($width,$height) = ($line =~ /(\d+)\s+(\d+)/); 
   print STDERR "DEBUG: width=$width, height=$height\n";

   # Read MAX PNM value and ignore
   $line = <STDIN>;

   # Read entire remainder of PNM file
   my $expectedsize=$width * $height * 3;
   my $PNMdata;
   my $bytesRead = read(STDIN,$PNMdata,$expectedsize);
   if($bytesRead != $expectedsize){die "Unable to read PNM data"}

   # Output PNG header chunk
   printf "\x89PNG\x0d\x0a\x1a\x0a";

   my $bitdepth=4;
   my $colortype=3;
   my $compressiontype=0;
   my $filtertype=0;
   my $interlacetype=0;

   # Output PNG IHDR chunk
   my $IHDR='IHDR';
   $IHDR .= pack 'N',$width;
   $IHDR .= pack 'N',$height;
   $IHDR .= pack 'c',$bitdepth;
   $IHDR .= pack 'c',$colortype;
   $IHDR .= pack 'c',$compressiontype;
   $IHDR .= pack 'c',$filtertype;
   $IHDR .= pack 'c',$interlacetype;
   &PNGoutputChunk($IHDR);

   #Output color space info
   my $sRGB = 'sRGB';
   $sRGB .= pack 'c', 0;
   &PNGoutputChunk($sRGB);
   my $gAMA = 'gAMA';
   $gAMA .= pack 'N', 45455;
   &PNGoutputChunk($gAMA);
   my $cHRM = 'cHRM';
   $cHRM .= pack 'N', 31270;
   $cHRM .= pack 'N', 32900;
   $cHRM .= pack 'N', 64000;
   $cHRM .= pack 'N', 33000;
   $cHRM .= pack 'N', 30000;
   $cHRM .= pack 'N', 60000;
   $cHRM .= pack 'N', 15000;
   $cHRM .= pack 'N',  6000;
   &PNGoutputChunk($cHRM);
   
   # Output PNG PLTE (palette)
   my $PLTE='PLTE';
   for(my $i=0;$i<scalar @palette;$i++){
      $PLTE .= sprintf('%c',$palette[$i][0]); # Red
      $PLTE .= sprintf('%c',$palette[$i][1]); # Green
      $PLTE .= sprintf('%c',$palette[$i][2]); # Blue
   }
   &PNGoutputChunk($PLTE);

   # Output PNG IDAT chunk
   # RFC-1950 zlib compression
   my $raw;
   # Go through PNM data, and for each RGB pixel, find nearest palette entry
   my @PNMvalues = unpack("C*",$PNMdata);
   print STDERR "Unpacked ",scalar @PNMvalues," from raw\n";
   my $mem=0;
   for(my $pixel=0;$pixel<(scalar @PNMvalues)/3;$pixel++){

      # Output filter type byte (0) at start of each scanline
      if($pixel%$width==0){$raw .= "\x00"; $mem=0;}

      my $r=$PNMvalues[(3*$pixel)];   # Red PNM value
      my $g=$PNMvalues[(3*$pixel)+1]; # Green PNM value
      my $b=$PNMvalues[(3*$pixel)+2]; # Blue PNM value
      my $nearest=0;
      my $distmin=(255*255)+(255*255)+(255*255); # Couldn't get further
      # Go through all palette entries to find nearest to this RGB
      for(my $pe=0;$pe<scalar @palette;$pe++){
         my $pr=$palette[$pe][0];   # Red palette value
         my $pg=$palette[$pe][1]; # Green palette value
         my $pb=$palette[$pe][2]; # Blue palette value
         my $dist = ($pr-$r)*($pr-$r) + ($pg-$g)*($pg-$g) + ($pb-$b)*($pb-$b);
         if($dist<$distmin){
            $distmin=$dist;
            $nearest=$pe;
         }
      }
      if(($pixel%$width)%2==0){
	  $mem=$nearest;
      }else{
	  $raw .= sprintf "%c",$mem*16+$nearest;
      }
      if(($pixel%$width)==($width-1)){
	  if($width%2){
	      $raw .= sprintf "%c",$mem*16;
	  }
      }
#     print STDERR "Pixel: $pixel, r=$r, g=$g, b=$b. Chose palette entry $nearest\n";
   }
   print STDERR "Length of raw: ",length($raw), "\n";

   my $deflated;
   my $status = deflate \$raw => \$deflated, Level => Z_BEST_COMPRESSION
        or die "deflate failed: $DeflateError\n";

   my $IDAT="IDAT" . $deflated;
   &PNGoutputChunk($IDAT);

   # Output PNG IEND chunk
   &PNGoutputChunk('IEND');
