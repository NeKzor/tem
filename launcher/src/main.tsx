/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

import { ThemeProvider } from '@material-tailwind/react';
import ReactDOM from 'react-dom/client';
import App from './App';
import './styles.css';

ReactDOM.createRoot(document.getElementById('root') as HTMLElement).render(
    <ThemeProvider>
        <App />
    </ThemeProvider>,
);
