$(function(){


  $("#control").css("display", "inline");

  var obsolete = false;
  var unicode = 1;
  var cata = true;
  var catc = true;
  var catg = true;
  var catl = true;
  var cato = true;
  var catx = true;
  var cats = true;

  function updateList()
  {
    $(".allitem").show();
    if ( !obsolete )
    {
      $(".obsolete").hide();
    }
    if ( unicode == 1 )
    {
      $(".ansi").hide();
      $(".x64").hide();
    }
    else if ( unicode == 2 )
    {
      $(".unicode").hide();
      $(".x64").hide();
    }
    else if ( unicode == 3 )
    {
      $(".x86").hide();
    }
    if ( !cata )
    {
      $(".categorya").hide();
    }
    if ( !catc )
    {
      $(".categoryc").hide();
    }
    if ( !catg )
    {
      $(".categoryg").hide();
      $(".categoryf").hide();
    }
    if ( !catl )
    {
      $(".categoryl").hide();
    }
    if ( !cato )
    {
      $(".categoryo").hide();
    }
    if ( !cats )
    {
      $(".categorys").hide();
    }
  }
  updateList();

  $("#swlatest").click(function(){
    obsolete = !obsolete;
    if ( obsolete )
    {
      $("#swlatest").text("最新のみ／[全て]");
    }
    else
    {
      $("#swlatest").text("[最新のみ]／全て");
    }
    updateList();
  });
  $("#swlatest").text("[最新のみ]／全て");
  $("#swunicode").click(function(){
    unicode = unicode + 1;
    if ( unicode == 1 )
    {
      $("#swunicode").text("[32bit]／ANSI／64bit／全て");
    }
    else if ( unicode == 2 )
    {
      $("#swunicode").text("32bit／[ANSI]／64bit／全て");
    }
    else if ( unicode == 3 )
    {
      $("#swunicode").text("32bit／ANSI／[64bit]／全て");
    }
    else
    {
      unicode = 0;
      $("#swunicode").text("32bit／ANSI／64bit／[全て]");
    }
    updateList();
  });
  $("#swunicode").text("[32bit]／ANSI／64bit／全て");
  $("#swcata").click(function(){
    cata = !cata;
    if ( cata )
    {
      $("#swcata").text("書庫[+]");
    }
    else
    {
      $("#swcata").text("書庫[-]");
    }
    updateList();
  });
  $("#swcatc").click(function(){
    catc = !catc;
    if ( catc )
    {
      $("#swcatc").text("設定[+]");
    }
    else
    {
      $("#swcatc").text("設定[-]");
    }
    updateList();
  });
  $("#swcatg").click(function(){
    catg = !catg;
    if ( catg )
    {
      $("#swcatg").text("一般[+]");
    }
    else
    {
      $("#swcatg").text("一般[-]");
    }
    updateList();
  });
  $("#swcatl").click(function(){
    catl = !catl;
    if ( catl )
    {
      $("#swcatl").text("カラム拡張[+]");
    }
    else
    {
      $("#swcatl").text("カラム拡張[-]");
    }
    updateList();
  });
  $("#swcato").click(function(){
    cato = !cato;
    if ( cato )
    {
      $("#swcato").text("出力[+]");
    }
    else
    {
      $("#swcato").text("出力[-]");
    }
    updateList();
  });
  $("#swcats").click(function(){
    cats = !cats;
    if ( cats )
    {
      $("#swcats").text("本体[+]");
    }
    else
    {
      $("#swcats").text("本体[-]");
    }
    updateList();
  });
});
