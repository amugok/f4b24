#! ruby

require 'csv'

print(<<"EOS")
<!doctype html>
<html lang="ja">
<head>
<meta charset="utf-8">
<title>f4b24</title>
<link rel="stylesheet" type="text/css" href="css/f4b24.css"></link>
<script type="text/javascript" src="js/jquery-2.1.3.min.js"></script>
<script type="text/javascript" src="js/controler.js"></script>
<script type="text/javascript">
</script>
</head>
<body>

<style type="text/css">
</style>

<div id="header">
<div id="header-bg">
<span class="h-tab"><a class="h-tab-link" href="#summary">f4b24</a></span>
<span class="h-tab"><a class="h-tab-link" href="#download">Download</a></span>
<span class="h-tab"><a class="h-tab-link" href="#link">Link</a></span>
</div>
</div>

<div id="body">

<div class="sec-header">
<div class="sec-header-cell">
<a name="summary">f4b24</a>
</div>
</div>

<div class="sec-header">
<div class="sec-header-cell">
<a name="download">Download</a>
</div>
</div>

<div id="control">
<span class="sw" id="swlatest">最新のみ／[全て]</span>
<span class="sw" id="swunicode">32bit／ANSI／64bit／[全て]</span>
<br>
<span class="sw" id="swcata">書庫[+]</span>
<span class="sw" id="swcatc">設定[+]</span>
<span class="sw" id="swcatg">一般[+]</span>
<span class="sw" id="swcatl">カラム拡張[+]</span>
<span class="sw" id="swcato">出力[+]</span>
<span class="sw" id="swcats">本体[+]</span>
<br>
</div>

<div id="table">

<div class="row">
<span class="collink">
ファイル名
</span>
<span class="colsize">
サイズ
</span>
<span class="colmtime">
更新日
</span>
</div>
EOS

table = CSV.open("filelist.tsv", :col_sep => "\t", :headers => true, :header_converters => :symbol, :converters=> lambda {|f| f ? f.strip : nil} )

table = table.find_all{|rx| (rx[6] && (!rx[0] || rx[0].to_s[0] != "#")) }

table = table.sort_by{|rx| rx[6]}

table.each {|rx|
	latest = rx[0] == "l" ? "latest" : "obsolete"
	unicode = rx[1] == "x" ? "unicode x64" : (rx[1] == "u" ? "unicode x86" : (rx[1] == "a" ? "ansi x86" : "common x86"))
	category = "category" + rx[2]
	size = rx[3]
	mtime = rx[4]
	classname = rx[5]
	filename = rx[6]
	tagname = rx[7] || "googlecode"
	releaseurl = rx[7] == "" ? "https://github.com/amugok/amugok.github.io/releases/download" : "https://github.com/amugok/f4b24/releases/download"

	print <<-"EOS"
<div class="row allitem #{latest} #{unicode} #{category}">
<span class="collink">
<a href="#{releaseurl}/#{tagname}/#{filename}">#{filename}</a>
</span>
<span class="colsize">
#{size}bytes
</span>
<span class="colmtime">
#{mtime}
</span>
</div>
	EOS

}

print(<<"EOS")
</div>

<div class="sec-header">
<div class="sec-header-cell">
<a name="link">Link</a>
</div>
</div>

</div>

</body>
</html>
EOS


