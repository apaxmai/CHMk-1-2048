
use warnings;
use strict;
use Test;
      
BEGIN { plan tests => 6 }

our @correctOutputs;
our $correctOutputs;
our @testStrings;
our $testStrings;

my $testOutput;
my $tests = 6;
my $i;

$testStrings[0] = 'Chymos Article Passphrase';
$correctOutputs[0] = '904d979e1e6fdd421057c0f3d29b0e5f8cbb7e345a49eeee6aa32a4c0f250be2ca03b1abc67805a47850d2ab72783fb3eaeee68254d3d03bc532a3fe18c134a0056c6f333467c358abc849c4dc5334858fcde04738a56e60600a824ed9f03cf67bc6d2871cc859091c3b4a6d18194749470fce25ce13689982d26036a9895c749f84b98465885a689ec520f435e5c7eb90e3460f00117218bffabea4cfe0dc74f221f6c3c517f498690abcc6fef3ade12b4c3c45348b0383314b36dc3061466da0a04fb28158932ec83f60b289db72861fb12c3f2f052b2fcd13945f66687530773906ea69e1556e8e27fb219a20789e0e9dc779b700977a620511474b6edf0b';

$testStrings[1] = 'abc123';
$correctOutputs[1] = '658f828b7b892a4e989fd6898473beeec22cad1c1dbc93471c7221d5086779f0f32e02afba8190cb6a8a345224cba54f7c75b0daf6f55ec82f8fcf7e131248b4da120e36a1e7827cd1f2f61ecdae0200696cb39d9d6597a1f33c5160f406e14b288236de4ffd196940bfc032ea7646bd5496b06e626c9dbca67188d582179e364f4023c87821bfae3d6888e674d40234ce9883d6306a23d83886fa551e03f7c23c3512944b27acecc64edb6b647816b4e6b8104d775803896f217c7f6a8a9819b1bb7a1b44b703b4ad431007aa099629488c54ff8cf421ffb935fb7477a8a79f91832670a3b799abecdbdea89a3c76516f542cb4819761d60f8190ae59c0e030';

$testStrings[2] = 'trustno1';
$correctOutputs[2] = '531cc998de41e4e75b8a168dbe5af99ac64748d6dd75c0f2b0d3666c544c410665fd639428c7384a03b4a8743239dbe933e7184b4904d9e45f91309fbaf75d097821bce8063c916cb108898ee4c7192da0eb6ec9108b046d3d6bcab3f2ecc52fbc9465d2a5231cb1f2b3dc90b0c856a7e291b091c22f5447ee9f6dfaf924d3da2b8059f962458f3f0357658f54d8744a75d110d831414474f3c771740d4ea3061460abf4af91da85bec3990c16f29ace60626d817617062ce105f925d1810522d034ccbe8455f0cb026eb59dc4c2737151edc3a09757851ff28aecb09b1f94f2495fc61ccf985750201ffe52ea24c33fbb62d1054b6b824495a6fabb30a28cd4';

$testStrings[3] = 'The quick brown fox jumps over the lazy dog';
$correctOutputs[3] = '38f700ec78ee60888cf0c44d59b3e8628af660ab7db3011a3cf3f941205828d2842dd6071c587e4950ac9b8f748fe13c0f3fbe5fdcc8701eba67535e2baba42c86be0003f3183f723cea3f14ab233299950ddf18294b770c42d5d23b0c29f325f93fac139ce33e2cf9dedef38f736af0fcbbccd56ca57dd5fa2c460153db39477529ea7f76b85d92ddbd405cd5d7724f8407fc92826afe495c7a2f01342e91ac38d370a092ab23e37faa776de14775b9a30202faf3a9e3e650303edc534e27fcd813a429e1d2c545ff120822cf23b271704a9f77a5ff15de3df8bccc1711030150a910a611f305d2471fb93c033e90450c3312ac2d3dc853af412a55be3ac23a';

$testStrings[4] = 'The quick brown fox jumps over the lazy cog';
$correctOutputs[4] = '1d34d074bc6a7f95a0f548904c44e1402c3ce8a9eef351bacd6a7c67a59d37ddb9d9dded248b695b21377ded37c82b7d332b910d349cc814aec7740583fb9227c8e72702b8f558a7861405f70add7fcd28a2ebf3dbc0b8927c38cc99f172b9bf60ca7cec25b5b4be4e39bfb88b43ac1187621ec3f14bbf822f7a0e9991232b344b0e5d4e86181bd74d4097e5c88c0e368ee6717ed7d77f4e24f33df54244cfed518ed22deffb6937384fbb0bfcf2da33c7b28fe21d214f407ad0a8b8c686708fabdefd69f2ac4d1385b8b9d2962f116db870139865c8595845a7dcf512548520f1eef1d7c21e16cc53dda57eae4e999519c2b2ef209af2e080399c873f6110b6';

$testStrings[5] = 'The fat professor jumps over the lazy professor';
$correctOutputs[5] = 'e4b1a19e21a4e3d38493dc6dbb2391520f3f6b68c5701e0acb4e22e8af56df8295032802e940314bada1a74b5fb2a44f1109e69f54d8977706cd66ecdba62bce19e95c99746ba58ec1be07cfd6b94cb1a91950f1a767623f9cfc39d0756d44195be6267c6978fe21e1bf33ee530571deb4fdadb4ccf613b13222f16d15a536483c59ea931045563e8ab029ece7a564f7c8e37718187ca082f81c5de9f22822ecd101e7db69c73ed3155998ef2fa1396c092dadf6fc8b08799dd4523138c330b891242c5fe6a55c8d473e1db5735065f7e0625d810dd79495c07f11a6926fe88b1d6e0fbc879be37c2adde05242485cac3081835ab1902c954d9fed79a8a35f75';

my $version;
local $/ = undef;
open (FH_IN, '<VERSION');
if ($version = <FH_IN>) {
	close (FH_IN);
} else {
	$version = "unknown ($!)";
}

print "Testing CHMk-1-2048 reference implementation ver. ${version}\n";

for ($i = 0; $i < $tests; $i++) {

	my $s = $testStrings[$i];

	print "CHMk-1-2048('$s')..";
	$testOutput = qx{echo -n "${s}" | ./CHMk-1-2048};
	printf ".\n";

	$testOutput =~ s/\n//g;

	ok($testOutput, $correctOutputs[$i]);
}

