import axios from 'redaxios';

// const BASE_URL = 'https://audioluxmockapi.azurewebsites.net';
// const BASE_URL = 'http://localhost:8000';
const BASE_URL = 'http://192.168.4.1';


// Load the API url from a static file. This file will be updated
// by the AudioLux firmware on boot or when a local WiFi network
// is joined.
//
let base_url;
async function get_base_url() {
    const load_url = async () => {
        try {
            const response = await fetch('../assets/url.json');
            const url_data = await response.json();
            return url_data.url;
        } catch (error) {
            return BASE_URL;
        }
    }

    base_url = await load_url()
}
get_base_url().then(_ => console.log(`base_url: ${base_url}`));



const getSettings = () =>
    getData('settings');


const getPatternList = () =>
    getData('patterns');


const getPattern = () =>
    getData('pattern');


const getWiFiList = () =>
    getData('wifis');

const getWiFi = () =>
    getData('wifi')

const getHostname = () =>
    getData('hostname');

const getHistory = () =>
    getData('history')


const getData = (path) =>
    axios.get(`${base_url}/api/${path}`, {headers: {'Access-Control-Allow-Origin': '*'}})
        .then(response => response.data);

const savePattern = (pattern) =>
    axios.put(`${base_url}/api/pattern`,{pattern});


const saveSettings = (settings) =>
    axios.put(`${base_url}/api/settings`, {...settings}, );

const joinWiFi = (wifi) =>
    axios.put(`${base_url}/api/wifi`,{...wifi});

const saveHostname = (hostname) =>
    axios.put(`${base_url}/api/hostname`,{hostname});

export {
    getSettings,
    saveSettings,
    getPatternList,
    getPattern,
    savePattern,
    getWiFiList,
    getWiFi,
    joinWiFi,
    getHostname,
    saveHostname,
    getHistory,
    base_url
};