import { authenticatedFetch } from "./auth.js";

document.getElementById('login').addEventListener('submit', async (event) => {
    event.preventDefault();
    const formData = new FormData(event.target);
    console.log("Submitting login form with username: " + formData.get("username") + " and password: " + formData.get("password"));
    try{
        const response = await fetch('/login', {
            method: 'POST',
            body: formData
        });
        console.log(response)
        if (response.ok) {
            console.log("Login successful ");
            window.location.href = "/"; // Redirect to home page on successful login
        } else {    
            const text = await response.text();
            console.log("Login failed, server response: " + text);
            alert("Login failed: " + text);
        }
    } catch (error) {
        console.error("Error during login request: ", error);
        alert("An error occurred during login. Please try again.");
    };
});
