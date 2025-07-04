var url = document.URL;

var uid = "undifined";
var nb_request = "undifined";

refreshText();
refresh_directory();
load_cookies();

function refreshText()
{
	document.getElementById("uid_text").textContent=`client uid: ${uid}`;
	document.getElementById("requests_text").textContent=`client requests: ${nb_request}`;

}

document.getElementById("refresh_btn").onclick=async() => {

	var response = await fetch('http://127.0.0.1:8008', {
		method: 'POST',
		headers: {
		'request_type': 'client_credentials',
		}
	}).catch(error => {console.error("Error:", error)});

	var data = await response.text();
	uid = data;

	var response = await fetch('http://127.0.0.1:8008', {
		method: 'POST',
		headers: {
		'request_type': 'client_visits',
		'UID': uid
		}
	}).catch(error => {console.error("Error:", error)});

	data = await response.text();
	nb_request = data;

	console.log(uid);
	console.log(nb_request);

	refreshText();

}

document.getElementById("send_btn").onclick=async() => {

	var file = document.getElementById("send_input").files[0];
	
	if (file)
	{
		// getting file content
		var content = await file.text();
		console.log(file);
		console.log(content);

		var response = await fetch('http://127.0.0.1:8008/uploads', {
			method: 'POST',
			headers:
			{
				'request_type': 'upload',
			},
			body: content
		}).catch(error => {
			document.getElementById("status").textContent = error;
			console.error("Error:", error)
		});
	
		data = await response.text();
		document.getElementById("status").textContent = data;
	}
	else
	{
		document.getElementById("status").textContent = "no file selected";
	}

	await refresh_directory();
}

async function refresh_directory()
{
	var response = await fetch('http://127.0.0.1:8008/uploads', {
		method: 'POST',
		headers: {
		'request_type': 'dir_content',
		}
	}).catch(error => {console.error("Error:", error)});
	
	var data = await response.text();
	
	// populate dir
	const files = data.split(";"); 
	console.log(files);
	
	document.getElementById("dir_content").innerHTML = "";
	files.forEach(element => {
		const newElement = document.createElement("a");
		newElement.href = `http://127.0.0.1:8008/uploads/${element}`
		newElement.textContent = element;
		document.getElementById("dir_content").appendChild(newElement);
		
	});

}

async function load_cookies()
{
	var data = document.cookie;
	
	// populate dir
	const files = data.split(";"); 
	console.log(files);
	
	document.getElementById("dir_content").innerHTML = "";
	files.forEach(element => {
		const newElement = document.createElement("p");
		newElement.textContent = element;
		document.getElementById("dir_content").appendChild(newElement);
		
	});

}

document.getElementById("refresh_dir_btn").onclick=async() => {
	await refresh_directory();
}

document.getElementById("delete_btn").onclick=async() => {

	file = document.getElementById("delete_input").value;
	console.log(file);
	if (file.text == "")
	{
		document.getElementById("status").textContent = "select a file";
		return;
	}
	url = `http://127.0.0.1:8008/uploads/${file}`
	
	var response = await fetch(url, {
		method: 'DELETE',
	}).catch(error => {
		document.getElementById("status").textContent = error;
		console.error("Error:", error)
	});
	
	var data = await response.text();
	document.getElementById("status").textContent = data;
	refresh_directory();
}