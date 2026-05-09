export async function authenticatedFetch(url, options = {}) {
  const token = sessionStorage.getItem('accessToken');
  const config = {
    ...options,
    headers: {
      ...options.headers,
      Authorization: token ? `Bearer ${token}` : '',
    },
  };
  return fetch(url, config);
};

export async function authenticatedXHR(method, url, body = null) {
  const token = sessionStorage.getItem('accessToken');
  const xhr = new XMLHttpRequest();
  xhr.open(method, url, true);
  xhr.setRequestHeader('Authorization', token ? `Bearer ${token}` : '');
  if (body) {
    xhr.setRequestHeader('Content-Type', 'application/json');
    xhr.send(JSON.stringify(body));
  } else {
    xhr.send();
  }
}