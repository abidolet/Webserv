var url = document.URL;

var uid = "undifined";
var nb_request = "undifined";

refreshText();

function refreshText()
{
	document.getElementById("uid_text").textContent=`client uid: ${uid}`;
	document.getElementById("requests_text").textContent=`client requests: ${nb_request}`;

}

function getSessionUID()
{
	fetch('http://127.0.0.1:8008', {
		method: 'POST',
		headers: {
		  'request_type': 'client_credentials'
		}
	  })
	.then(response => response.text())
	.then(data => {
		uid = data;
		console.log(data);
		refreshText();
	})
	.catch(error => {
		console.error('Error:', error);
	});
	
}


function getSessionRequests()
{
	fetch('http://127.0.0.1:8008', {
		method: 'POST',
		headers: {
		  'request_type': 'client_visits',
		  'UID': uid
		}
	  })
	.then(response => response.text())
	.then(data => {
		nb_request = data;
		console.log(data);
		refreshText();
	})
	.catch(error => {
		console.error('Error:', error);
	});

}