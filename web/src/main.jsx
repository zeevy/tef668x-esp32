import { render } from 'preact';
import './styles.css';
import { App } from './app.jsx';
import { startLive } from './data/radio.js';

startLive();
render(<App />, document.getElementById('app'));
