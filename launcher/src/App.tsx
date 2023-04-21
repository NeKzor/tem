/*
 * Copyright (c) 2023, NeKz
 *
 * SPDX-License-Identifier: MIT
 */

import { createElement, useCallback, useEffect, useMemo, useReducer, useState } from 'react';
import { Tabs, TabsHeader, TabsBody, Tab, TabPanel } from '@material-tailwind/react';
import { CommandLineIcon, WrenchIcon, RocketLaunchIcon } from '@heroicons/react/24/solid';
import Footer from './components/Footer';
import Settings from './views/Settings';
import Launcher from './views/Launcher';
import Console from './views/Console';
import AppState, { AppReducer, DispatchAction, loadState } from './AppState';
import { createLauncherConfig } from './Types';

function App() {
    const [state, dispatch] = useReducer(...AppReducer);
    const [gameLaunched, setGameLaunched] = useState(false);

    useEffect(() => {
        loadState().then((state) => {
            if (state) {
                console.log('found config');

                const configs = state.configs?.map(createLauncherConfig) ?? [];
                dispatch(DispatchAction.LoadConfig({ ...state, configs }));

                const config = configs.find(({ isDefault }) => isDefault);
                if (config) {
                    dispatch(DispatchAction.LaunchConfig({ config, onComplete: () => setGameLaunched(false) }));
                }
            }
        });
    }, []);

    const onGameLaunched = useCallback((value: boolean) => setGameLaunched(value), [setGameLaunched]);

    const context = useMemo(() => ({ state, dispatch }), [state, dispatch]);

    return (
        <AppState.Provider value={context}>
            <div className="flex flex-col h-screen justify-between">
                <main>
                    <Tabs value="tem">
                        <TabsHeader>
                            <Tab key="tem" value="tem">
                                <div className="flex items-center gap-2">
                                    {createElement(RocketLaunchIcon, { className: 'w-5 h-5' })}
                                    Launcher
                                </div>
                            </Tab>
                            <Tab key="console" value="console">
                                <div className="flex items-center gap-2">
                                    {createElement(CommandLineIcon, { className: 'w-5 h-5' })}
                                    Console
                                </div>
                            </Tab>
                            <Tab key="settings" value="settings">
                                <div className="flex items-center gap-2">
                                    {createElement(WrenchIcon, { className: 'w-5 h-5' })}
                                    Settings
                                </div>
                            </Tab>
                        </TabsHeader>
                        <TabsBody>
                            <TabPanel key="tem" value="tem">
                                <Launcher gameLaunched={gameLaunched} onGameLaunched={onGameLaunched} />
                            </TabPanel>
                            <TabPanel key="console" value="console">
                                <Console />
                            </TabPanel>
                            <TabPanel key="settings" value="settings">
                                <Settings />
                            </TabPanel>
                        </TabsBody>
                    </Tabs>
                </main>
                <Footer />
            </div>
        </AppState.Provider>
    );
}

export default App;
