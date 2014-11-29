<script type="text/javascript">
var xmlHttp;
function createXMLHttpRequest(){
	if(window.ActiveXObject){
		xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
	}
	else if(window.XMLHttpRequest){
		xmlHttp = new XMLHttpRequest();
	}
}
function start(){
	createXMLHttpRequest();
	var url="top.php";
	xmlHttp.open("GET",url,true);
	xmlHttp.onreadystatechange = callback;
	xmlHttp.send(null);
}
function callback(){
	if(xmlHttp.readyState == 4){
		if(xmlHttp.status == 200){
			document.getElementById("show").innerHTML = xmlHttp.responseText;
			setTimeout("start()",1000);
		}
	}
}
start();
</script>

<html>
<head>
<meta http-equiv="content-type" content="text/html;charset=utf8" />
</head>
<body>
<p><font color="red"><span id="show"></span></font></p>
</body>
</html>
