// event listner
document.getElementById('show_hide_password').addEventListener("click", () => show_password('password', 'eye', 'eye_slash'))


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
function setup_mqtt(_mqtt, _user, _topic, _password) {
    if (_topic == "") {
        _topic = "home/garage/"
    }
    var mqtt_data = {
        ip: _mqtt,
        user: _user,
        topic: _topic,
        password: _password
    };
    send_json(mqtt_data, '/mqtt');
    clear_field('mqtt', 'user', 'topic', 'password');

}
// clear text field
function clear_field(i, x, y, z) {
    if (y != null) {
        document.getElementById(i).value = "";
        document.getElementById(x).value = "";
        document.getElementById(y).value = "";
        document.getElementById(z).value = "";
        return
    };
    document.getElementById(i).value = "";
    document.getElementById(x).value = "";
    i = "";
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