<!DOCTYPE html>
<html>
	<head>
		<link rel="stylesheet" href="./codebase/webix.css" type="text/css" media="screen" charset="utf-8">
		<script src="./codebase/webix.js" type="text/javascript" charset="utf-8"></script>
		<link rel="stylesheet" type="text/css" href="./common/samples.css">
		<style>
			#areaA, #areaB {
				margin: 50px 10px;
				width:320px;
				height:400px;
				float: left;
			}
			#areaC{
				margin: 50px 10px;
				width:800px;
				height:400px;
				float: left;			
			}
		</style>
		<style type="text/css">
			.webix_tree_error{
				background-image: url(./myicons/error.png)
			}
		</style>		
		<title>Tabview</title>
	</head>
	<body>
		<script type="text/javascript">
		</script>	
		<div id="areaA"></div>
		<table>
		<tr><input id = "Start" type="button" value="Start" /><input id = "Stop" type="button" value="Stop" /><input id = "Kill" type="button" value="Kill" /></tr>
		<tr><td>
			<div id = "areaC" style="background:black">
			<p><font color="white"><span id="processInfo"></span></font></p>
			</div>
		</td></tr>
		<tr><p><font color="black"><span id="ServiceDesp"></span></font></p></tr>		
		</table>		
		<script type="text/javascript" charset="utf-8">
			var filter = null;
			var xmlHttp = null;
			var PhysicalView = null;
			var LogicalView = null;
			var DeployPhyTree = null;
			var DeployLogTree = null;
			var PhysicalTree = null;
			var LogicalTree = null;
			var phyInit = false;
			var logInit = false;
			var firstrun = true;
			var CurrentView = null;
			var physelectID = null;
			var logselectID = null;
			function ShowStatus(){
				var selectID = null;
				if(CurrentView == PhysicalView){
					selectID = physelectID;
				}else{
					selectID = logselectID;
				} 
				if(!selectID){
					selectID = CurrentView.getFirstId();
					CurrentView.select(selectID);
				}
				var str = '';
				if(selectID){
					var item = CurrentView.getItem(selectID);
					var root;
					if(item.root){
						root = item;
					}else{
						root = CurrentView.getItem(CurrentView.getParentId(selectID));
					}
					if(root){
						if(CurrentView == PhysicalView){ 
							var machine = PhysicalTree[root.value];
							if(machine){
								var status = machine.machine;
								for(var i = 0,len = status.length; i < len; i++){
									str = str + status[i] + '</br>';
								}
								str = str + "-------------------------------process------------------------------- </br>";
								var process = machine.process;
								if(process){
									if(root.id == selectID){											
										for (var key in process){
											str = str + "pid:" + process[key].pid + ",usr:" + process[key].usr;
											str = str + ",cpu:" + process[key].cpu + ",mem:" + process[key].mem;
											str = str + ",cmd:" + process[key].cmd + "</br>";
										}			
									}else{
										str = str + "pid:" + process[item.value].pid + ",usr:" + process[item.value].usr;
										str = str + ",cpu:" + process[item.value].cpu + ",mem:" + process[item.value].mem;
										str = str + ",cmd:" + process[item.value].cmd + "</br>";
									}
								}												
							}
						}else{
							var group = LogicalTree[root.value];
							if(group){
								if(root.id == selectID){											
									for (var key in group){
										str = str + "pid:" + group[key][1].pid + ",usr:" + group[key][1].usr;
										str = str + ",cpu:" + group[key][1].cpu + ",mem:" + group[key][1].mem;
										str = str + ",cmd:" + group[key][1].cmd + "</br>";
									}			
								}else{
									str = str + "pid:" + group[item.value][1].pid + ",usr:" + group[item.value][1].usr;
									str = str + ",cpu:" + group[item.value][1].cpu + ",mem:" + group[item.value][1].mem;
									str = str + ",cmd:" + group[item.value][1].cmd + "</br>";
								}		
							}
						}	
					}
				}
				document.getElementById("processInfo").innerHTML = str;
			}						
			function createXMLHttpRequest(){
				if(window.ActiveXObject){
					xmlHttp = new ActiveXObject("Microsoft.XMLHTTP");
				}
				else if(window.XMLHttpRequest){
					xmlHttp = new XMLHttpRequest();
				}
			}
			function fetchdata(){
				createXMLHttpRequest();
				var url="info.php";
				xmlHttp.open("GET",url,true);
				xmlHttp.setRequestHeader("Content-Type","application/x-www-form-urlencoded; charset=UTF-8");		
				xmlHttp.onreadystatechange = callback;
				xmlHttp.send(null);
			}
			function buildPhyTree(machinedata){
				function match(name,array){
					for(var i = 0,len = array.length; i < len; ++i){
						if(array[i].cmd.search(name) > 0){
							return array[i];									
						}
					}
					return null;
				}
				PhysicalTree = {};
				for(var i = 0,len1 = machinedata.length; i < len1; i++){
					PhysicalTree[machinedata[i].ip] = {};
					var tmp1 = PhysicalTree[machinedata[i].ip];
					tmp1.machine	= machinedata[i].status[0];
					tmp1.process = {};
					var tmp2 = DeployPhyTree[machinedata[i].ip]
					var tmp3 = machinedata[i].status[1];
					for(var j = 0,len2 = tmp2.length; j < len2;j++){
						var process = match(tmp2[j],tmp3);
						if(process){
							tmp1.process[tmp2[j]] = process;
						}
					}	
				}
			}
			function buildLogicalTree(){
				LogicalTree = {};
				for (var key1 in DeployLogTree){
					for(var key2 in PhysicalTree){
						var ip = key2;
						var process = PhysicalTree[key2].process;
						for(var key3 in process){
							if(process[key3].cmd.search(key1) > 0){
								if(!LogicalTree[key1]){
									LogicalTree[key1] = {};
								}
								LogicalTree[key1][key3] = [ip,process[key3]];			
							}
						}
					}
				}				
			}
			function buildDeployPhyTree(deploydata){
				DeployPhyTree = {};
				for(var i = 0,len1 = deploydata.length; i < len1; i++){
					var tmp1 = deploydata[i].service;
					for(var j = 0,len2 = tmp1.length; j < len2; j++){
						var tmp2 = DeployPhyTree[tmp1[j].ip];
						if(!tmp2){
							tmp2 = [];
							DeployPhyTree[tmp1[j].ip] = tmp2;
						}
						tmp2.push(tmp1[j].logicname);	
					}		
				}
			}
			function buildDeployLogTree(deploydata){
				DeployLogTree = {};
				for(var i = 0,len1 = deploydata.length; i < len1; i++){
					var tmp1 = deploydata[i].service;
					var tmp2 = [];
					DeployLogTree[deploydata[i].groupname] = tmp2;
					for(var j = 0,len2 = tmp1.length; j < len2; j++){
						tmp2.push(tmp1[j].logicname);
					}								
				}								
			}
			function buildPhyView(){
				for (var key in DeployPhyTree){
					var services = DeployPhyTree[key];
					var rootitem = {value:key,root:true};
					var rootid = PhysicalView.add(rootitem, null, 0);
					var err = false;					
					var goterror = false;
					for(var i = 0,len = services.length; i < len; i++){
						var item;
						var icon;
						var ip = key;
						var logicname = services[i];
						if(PhysicalTree[ip] && PhysicalTree[ip].process[logicname]){
							icon = "";
						}else{
							icon = "error";
							goterror = true;
						}
						item = {value:services[i],icon:icon};
						var id = PhysicalView.add(item, null, rootid);
					}
					if(goterror){
						PhysicalView.updateItem(rootid, {value:key,root:true,icon:"error"});
					}					
				}
			}						
			function buildLogView(){
				for (var key in DeployLogTree){
					var tmp1 = DeployLogTree[key];
					var rootitem = {value:key,root:true};
					var rootid = LogicalView.add(rootitem, null, 0);
					var goterror = false;
					for(var j = 0,len2 = tmp1.length; j < len2; j++){
						var item; 
						var icon;
						var group = key;
						var logicname = tmp1[j];
						if(LogicalTree[group] && LogicalTree[group][logicname]){
							icon = "";
						}else{
							icon = "error";
							goterror = true;
						}
						item = {value:tmp1[j],icon:icon};						
						var id = LogicalView.add(item, null, rootid);					
					}
					if(goterror){
						LogicalView.updateItem(rootid, {value:key,root:true,icon:"error"});
					}					
				}
			}
			function updatePhyView(){

			}
			function updateLogView(){

			}
			function callback(){
				if(xmlHttp.readyState == 4){
					if(xmlHttp.status == 200){
						var info = JSON.parse(xmlHttp.responseText);
						var deploydata = info.deployment;
						var machinedata = info.machine_status;																
						if(firstrun){
							buildDeployPhyTree(deploydata);
							buildPhyTree(machinedata);
							buildPhyView();	
							buildDeployLogTree(deploydata);
							buildLogicalTree();
							buildLogView();
						}else{
							buildPhyTree(machinedata);
							buildLogicalTree();
						}
						ShowStatus();				
						firstrun = false;
						setTimeout("fetchdata()",1000);
					}
				}
			}						
			webix.ui({
				id:"LeftTree",
				container: "areaA",
				borderless:true, 
				view:"tabview",							
				cells:[
					{
						header:"Physical View",							
						body:{
							select:true,
							view:"tree",
							ready:function(){ 
								PhysicalView = this;
								CurrentView = PhysicalView;
								phyInit = true;
								if(logInit){
									fetchdata();
								}
							},							
							on:{
								"onAfterSelect":function(id){
									physelectID = id;
									fetchdata();
								}
							},
							data:[]
						},
					},
					{
						header:"Logical View",
						body:{
							select:true,
							view:"tree",
							ready:function(){ 
								LogicalView = this;
								logInit = true;
								if(phyInit){
									fetchdata();
								}								
							},							
							on:{
								"onAfterSelect":function(id){
									logselectID = id;
									fetchdata();
								}
							},
							data:[]
						},
					},							
				]
			});
			$$("LeftTree").getChildViews()[1].attachEvent("onViewChange",function(id){
			        if(CurrentView == LogicalView)
			        	CurrentView = PhysicalView;
			       else
			       	CurrentView = LogicalView;
			       fetchdata();
			});
			
		</script>
	</body>
</html>