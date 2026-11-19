(function(){
var q=document.getElementById('q'),res=document.getElementById('results'),
 body=document.getElementById('content');
function esc(s){return s.replace(/[&<>"]/g,function(c){return{'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;'}[c];});}
function hl(s,terms){s=esc(s);terms.forEach(function(t){if(!t)return;
 var re=new RegExp('('+t.replace(/[.*+?^${}()|[\]\\]/g,'\\$&')+')','gi');
 s=s.replace(re,'<mark>$1</mark>');});return s;}
function snippet(text,terms){var low=text.toLowerCase(),at=-1;
 for(var i=0;i<terms.length;i++){at=low.indexOf(terms[i]);if(at>=0)break;}
 var a=Math.max(0,at-80),b=Math.min(text.length,(at<0?0:at)+180);
 return (a>0?'... ':'')+text.slice(a,b)+(b<text.length?' ...':'');}
function search(v){var terms=v.toLowerCase().split(/\s+/).filter(Boolean);
 if(!terms.length){res.style.display='none';body.style.display='';return;}
 var hits=[];DRU_INDEX.forEach(function(s){var t=(s.h+' '+s.t).toLowerCase(),
  score=0;for(var i=0;i<terms.length;i++){var n=t.split(terms[i]).length-1;
  if(!n)return;score+=n+(s.h.toLowerCase().indexOf(terms[i])>=0?20:0);}
  hits.push([score,s]);});
 hits.sort(function(a,b){return b[0]-a[0];});
 var out='<h1>Search: '+esc(v)+'</h1><p>'+hits.length+' section(s) found.</p>';
 hits.slice(0,60).forEach(function(h){var s=h[1];
  out+='<div class="r"><a href="'+s.u+'">'+hl(s.h,terms)+'</a><div class="where">'+
  esc(s.p)+'</div><div class="snip">'+hl(snippet(s.t,terms),terms)+'</div></div>';});
 res.innerHTML=out;res.style.display='block';body.style.display='none';}
var timer;q.addEventListener('input',function(){clearTimeout(timer);
 timer=setTimeout(function(){search(q.value.trim());
 try{history.replaceState(null,'',q.value.trim()?'?q='+encodeURIComponent(q.value.trim()):location.pathname+location.hash);}catch(e){}},120);});
q.addEventListener('keydown',function(e){if(e.key==='Escape'){q.value='';search('');}});
document.addEventListener('keydown',function(e){if(e.key==='/'&&document.activeElement!==q){e.preventDefault();q.focus();}});
var m=/[?&]q=([^&]*)/.exec(location.search);if(m){q.value=decodeURIComponent(m[1].replace(/\+/g,' '));search(q.value);}
var mb=document.getElementById('menu-btn');mb.addEventListener('click',function(){
 document.querySelector('nav').classList.toggle('open');});
var root=document.documentElement;
try{var th=localStorage.getItem('dru-theme');if(th)root.setAttribute('data-theme',th);}catch(e){}
document.getElementById('theme-btn').addEventListener('click',function(){
 var dark=root.getAttribute('data-theme')==='dark'||(!root.getAttribute('data-theme')&&
 matchMedia('(prefers-color-scheme: dark)').matches);var n=dark?'light':'dark';
 root.setAttribute('data-theme',n);try{localStorage.setItem('dru-theme',n);}catch(e){}});
})();
