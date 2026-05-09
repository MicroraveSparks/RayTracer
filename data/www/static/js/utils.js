export async function checkSafety() {
    let response = await fetch('/safety-status')
    .then(response => {
        response.json().then(data => {
            return Promise.resolve(data['safety']);
        }).catch(error => {
            console.error("Error parsing JSON response: ", error);
            return Promise.reject(error);
        });
    }).catch(error => {
        console.error("Error fetching safety status: ", error);
        return Promise.reject(error);
    });
}

export async function refreshSafety() {
    checkSafety().then(status => {
        const statusElement = document.getElementById('safety-status');
        if (status === 'safe') {
            statusElement.textContent = 'Safety Status: SAFE';
            statusElement.classList.remove('alert-danger');
            statusElement.classList.remove('alert-light');
            statusElement.classList.remove('alert-warning');
            statusElement.classList.add('alert-success');
        } else if (status === 'unsafe') {
            statusElement.textContent = 'Safety Status: UNSAFE';
            statusElement.classList.remove('alert-success');
            statusElement.classList.remove('alert-light');
            statusElement.classList.remove('alert-warning');
            statusElement.classList.add('alert-danger');
        } else if (status === 'paused') {
            statusElement.textContent = 'Safety Status: PAUSED';
            statusElement.classList.remove('alert-success');
            statusElement.classList.remove('alert-danger');
            statusElement.classList.remove('alert-warning');
            statusElement.classList.add('alert-light');
        } else {
            statusElement.textContent = 'Safety Status: ERROR';
            statusElement.classList.remove('alert-success');
            statusElement.classList.remove('alert-danger');
            statusElement.classList.remove('alert-light');
            statusElement.classList.add('alert-warning');
        }
    }).catch(error => {
        console.error("Error checking safety status: ", error);
        const statusElement = document.getElementById('safety-status');
        statusElement.textContent = 'Safety Status: ERROR';
        statusElement.classList.remove('alert-success');
        statusElement.classList.remove('alert-danger');
        statusElement.classList.remove('alert-light');
        statusElement.classList.add('alert-warning');
    });
}