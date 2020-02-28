var data = [];
var labels = [];
var ctx;
var chart;
var maxY = 26;
var minY = 0;
var minTemp;
var maxTemp;

var updateTemperature;
var stopFlag = false;
window.onload = async function()
{	
	try{
		$("#submit")[0].disabled = true;
		//append wifiNetwork container
		response = await fetch("/getWifiList");
		t = await response.text();
		console.log(t);
		$("#inputGroupSelect01").empty();
		$("#inputGroupSelect01").append(t);
		$("#submit")[0].disabled = false;
		var wifilist = setInterval(await getWiFiList,5000);

		$("#submit").on("click",function(){
		let SSID = $("#inputGroupSelect01").val();
		let pass = $("#exampleInputPassword1").val();
		fetch("/configure?SSID="+SSID+"&pass="+pass);
		});

		$("#inputGroupSelect01").on("change",function(){
			clearInterval(wifilist);
		})
		
	}
	catch
	{
		this.console.log("Режим теромометра");
	}
	this.updateChart();
	updateTemperature =  setInterval(await getTemerature,2000);
}

async function getWiFiList()
{
	response = await fetch("/getWifiList");
	t = await response.text();
	console.log(t);
	$("#inputGroupSelect01").empty();
	$("#inputGroupSelect01").append(t);
	$("#submit")[0].disabled = false;
}
async function getTemerature()
{
	
	response = await fetch("/getTemperature");
	t = await response.text();
	console.log(t);
	if(t == "-127.00")
	{
			$('.tValue')[0].innerText = "N/C";
			return;
	}

	var date = new Date();
	var str = date.getHours()+":"+date.getMinutes()+":"+date.getSeconds();

	if($(".tValue")[0].style.fontSize != "50px"){$(".tValue")[0].style.fontSize = "50px"}
	$('.tValue')[0].innerText = t;
	data.push(t);
	labels.push(str);

	var value = parseFloat(t);

    if(value > Math.max.apply(null,data))
	{
		maxY = value;
		updateChart();
	}
	else 
		if(value < Math.min.apply(null,data))
		{
			minY = value;
			updateChart();
		}
		
	$('.maxValue')[0].innerText = Math.max.apply(null,data);
	$('.minValue')[0].innerText = Math.min.apply(null,data);
	
	chart.update();
}
function removeData() {
	while(data.length){
		data.pop();
		labels.pop();
	}
    chart.update();
}
async function toogle(element)
{

	console.log(element)
	if($("#"+element.id).hasClass("btn-danger"))//остановлено
	{
		$("#"+element.id)[0].innerText = "Останов.";
		$("#"+element.id).removeClass("btn-danger");
		$("#"+element.id).addClass("btn-primary");
		updateTemperature = setInterval(await getTemerature,2000);
	}
	else
	{
		$("#"+element.id)[0].innerText = "Пуск";
		$("#"+element.id).removeClass("btn-primary");
		$("#"+element.id).addClass("btn-danger");
		clearInterval(updateTemperature);
	}
}

function updateChart()
{

	ctx = document.getElementById('myChart').getContext('2d');
	chart = new Chart(ctx, {
		// The type of chart we want to create
		type: 'line',
		
		// The data for our dataset
		data: {
			labels: labels,
			datasets: [{
				label: 'Динамика температуры',
				backgroundColor: 'rgb(255, 99, 132)',
				borderColor: 'rgb(255, 99, 132)',
				data: data,
				fill: false,
			}],
		},options: {
			scales: {
				xAxes: [{
					display: true,
					scaleLabel: {
						display: true,
						labelString: 'Время замера'
					}
				}],
				yAxes: [{
					display: true,
					scaleLabel: {
						display: true,
						labelString: 'Температура °C'
					},
					ticks:
					{
						min:this.minY,
						max:this.maxY,
						stepSize:(Math.abs(this.minY)+Math.abs(this.maxY))/10
					}
				}]
			}
		}
	});
}