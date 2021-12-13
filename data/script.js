// event listner
document.getElementById('show_hide_password').addEventListener("click", () => show_password('password', 'eye', 'eye_slash'))
document.getElementById('door_closed').addEventListener("click", () => operate_door('door_open', 'door_closed'))
document.getElementById('door_open').addEventListener("click", () => operate_door('door_closed', 'door_open'))


// show/hide password 
function show_password(password, eye, eye_slash) {
    password = document.getElementById(password);
    eye = document.getElementById(eye);
    eye_slash = document.getElementById(eye_slash);
    if (password.type === "password") {
        password.type = "text";
        eye.style.display = "none";
        eye_slash.style.display = "block";
        return
    }
    password.type = "password";
    eye.style.display = "block";
    eye_slash.style.display = "none";
}


function clean(e) {
    var textfield = document.getElementById(e);
    var regex = /[^0-9.:]/gi;
    textfield.value = textfield.value.replace(regex, "");
}

function operate_door(_visable, _invisable) {
    document.getElementById(_visable).style.display = "block";
    document.getElementById(_invisable).style.display = "none";
    _url = '/?' + _visable;
    var xhr = new XMLHttpRequest();
    xhr.open('GET', _url, true);
    xhr.send()

}


// setup JSON for SSID
function setup_network(_ssid, _password) {
    var ssid_data = {
        ssid: _ssid,
        password: _password
    };
    send_json(ssid_data, 'wifi');
    clear_field('ssid', 'password');

}

// setup JSON for MQTT
function setup_mqtt(_mqtt, _port, _user, _topic, _password) {
    if (_topic == "") {
        _topic = "homeassistant/cover/garage/"
    }
    var mqtt_data = {
        ip: _mqtt,
        port: _port,
        user: _user,
        topic: _topic,
        password: _password
    };
    send_json(mqtt_data, '/mqtt');
    clear_field('mqtt', 'port', 'user', 'topic', 'password');

}
// clear text field
function clear_field(i, j, x, y, z) {
    if (x != null) {
        document.getElementById(i).value = "";
        document.getElementById(j).value = "";
        document.getElementById(x).value = "";
        document.getElementById(y).value = "";
        document.getElementById(z).value = "";
        return
    };
    document.getElementById(i).value = "";
    document.getElementById(j).value = "";
    i = "";
    j = "";
    x = "";
    y = "";
    z = "";
}
// post JSON
function send_json(data, _file) {
    _url = _file + '.json';
    console.log(data, _url);
    var xhr = new XMLHttpRequest();

    xhr.open('POST', _url, true);
    xhr.send(JSON.stringify(data))
}
setInterval(function() {
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function() {
        if (this.readyState == 4 && this.status == 200) {
            console.log(this.responseText);
            if (this.responseText == "open") {
                document.getElementById('door_open').style.display = 'block';
                document.getElementById('door_closed').style.display = 'none';
            } else {
                document.getElementById('door_closed').style.display = 'block';
                document.getElementById('door_open').style.display = 'none';
            }
        }
    };
    xhr.open("GET", "/garage_state", true);
    xhr.send();
}, 1000);