#!/usr/bin/perl

my $in_dir = shift @ARGV;
my $out_dir = shift @ARGV;

die "needs 2 arguments" unless ($in_dir && $out_dir);

system("rm -f $out_dir/*.zip");
pack_gerbers($in_dir, $out_dir);


sub pack_gerbers
{
	my($in_dir, $out_dir) = @_;
	my $dh;
	opendir($dh, $in_dir) || die $!;
	my @files = readdir($dh);
	closedir($dh);
	
	my $gbr_files = {};
	foreach my $file(@files)
	{
		next if $file =~ /^\./;
		if (-d "$in_dir/$file")
		{
			pack_gerbers("$in_dir/$file", "$out_dir");
			next;
		}
		elsif (-r "$in_dir/$file" && $file =~ /(.*?)-(dimension|top|bottom|drill|tstop|bstop|ttext|btext)\.gbr$/)
		{
			$gbr_files->{$1} = {} unless $gbr_files->{$1};
			$gbr_files->{$1}->{$2} = $file;
		}
	}
	
	return if scalar(keys %{$gbr_files}) == 0;
	system("mkdir -p '$out_dir'");
	my $cd = `pwd`;
	chomp($cd);
	while (my($key, $files) = each %{$gbr_files})
	{
		print "creating $key.zip ...\n";
		mkdir("$out_dir/$key") || die $!;
		copy_file($in_dir, $out_dir, $files->{dimension}, "$key/$key.do");
		copy_file($in_dir, $out_dir, $files->{top}, "$key/$key.gtl");
		copy_file($in_dir, $out_dir, $files->{bottom}, "$key/$key.gbl");
		copy_file($in_dir, $out_dir, $files->{drill}, "$key/$key.txt");
		copy_file($in_dir, $out_dir, $files->{tstop}, "$key/$key.gts");
		copy_file($in_dir, $out_dir, $files->{bstop}, "$key/$key.gbs");
		copy_file($in_dir, $out_dir, $files->{ttext}, "$key/$key.gto");
		copy_file($in_dir, $out_dir, $files->{btext}, "$key/$key.gbo");
		my $rf = join(' ', @$results);
		system("cd '$out_dir' && zip -9r '$key.zip' $key && rm -rf $key && cd '$cd'");
		print "ok\n";
	}
}

sub copy_file
{
	my ($in_dir, $out_dir, $source, $dest) = @_;
	return unless $source;
	print "  $source -> $dest\n";
	system("cp -f '$in_dir/$source' '$out_dir/$dest'");
	die "Copy error $?" if $?;
}

