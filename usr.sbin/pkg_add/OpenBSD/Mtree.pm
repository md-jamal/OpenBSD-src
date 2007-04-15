# ex:ts=8 sw=4:
# $OpenBSD: Mtree.pm,v 1.3 2007/04/15 10:17:29 espie Exp $
#
# Copyright (c) 2004-2005 Marc Espie <espie@openbsd.org>
#
# Permission to use, copy, modify, and distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

use strict;
use warnings;

package OpenBSD::Mtree;
use File::Spec;

# read an mtree file, and produce the corresponding directory hierarchy

sub parse_fh
{
	my ($mtree, $basedir, $fh) = @_;
	local $_;
	while(<$fh>) {
		chomp;
		s/^\s*//;
		next if /^\#/ || /^\//;
		s/\s.*$//;
		next if /^$/;
		if ($_ eq '..') {
			$basedir =~ s|/[^/]*$||;
			next;
		} else {
			$basedir.="/$_";
		}
		$_ = $basedir;
		while (s|/\./|/|)	{}
		$mtree->{File::Spec->canonpath($_)} = 1;
	}
}

sub parse 
{
	my ($mtree, $basedir, $filename) = @_;
	open my $file, '<', $filename or die "can't open $filename: $!";
	parse_fh($mtree, $basedir, $file);
	close $file;
}

1;
