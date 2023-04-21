import { Dispatch, Reducer, SetStateAction, createContext } from 'react';
import { CheckForUpdatesOption, LauncherConfig, SortOption } from './Types';
import { BaseDirectory, readTextFile, writeTextFile } from '@tauri-apps/api/fs';
import { invoke } from '@tauri-apps/api';

const configFile = 'config.json';

export interface AppState {
    configs: LauncherConfig[];
    sort: SortOption;
    checkForUpdates: CheckForUpdatesOption;
}

const initialState: AppState = {
    configs: [],
    sort: {
        key: 'createdAt',
        direction: 'asc',
    },
    checkForUpdates: 'disabled',
};

export enum DispatchActionKey {
    SetConfigs,
    SetSort,
    SetCheckForUpdates,
    LoadConfig,
    SaveConfig,
    LaunchConfig,
}

export type LauncherConfigPayload = { config: LauncherConfig; onComplete?: () => void };

export const DispatchAction = Object.freeze({
    SetConfigs: (payload: SetStateAction<LauncherConfig[]>) => ({ action: DispatchActionKey.SetConfigs, payload }),
    SetSort: (payload: SetStateAction<SortOption>) => ({ action: DispatchActionKey.SetSort, payload }),
    SetCheckForUpdates: (payload: SetStateAction<CheckForUpdatesOption>) => ({
        action: DispatchActionKey.SetCheckForUpdates,
        payload,
    }),
    LoadConfig: (payload: SetStateAction<Partial<AppState>>) => ({ action: DispatchActionKey.LoadConfig, payload }),
    SaveConfig: () => ({ action: DispatchActionKey.SaveConfig }),
    LaunchConfig: (payload: LauncherConfigPayload) => ({ action: DispatchActionKey.LaunchConfig, payload }),
});

export interface DispatchPayload<T = unknown> {
    action: DispatchActionKey;
    payload?: SetStateAction<T>;
}

export const loadState = async () => {
    return await readTextFile(configFile, { dir: BaseDirectory.AppLocalData })
        .then((text) => JSON.parse(text) as Partial<AppState>)
        .catch((err) => {
            console.log(`failed to read ${configFile}`, err);
        });
};

export const saveState = async (state: AppState) => {
    await writeTextFile(configFile, JSON.stringify(state), { dir: BaseDirectory.AppLocalData })
        .then(() => {
            console.log('saved config');
        })
        .catch((err) => {
            console.log(`failed to write ${configFile}`, err);
        });
};

const reducer: Reducer<AppState, DispatchPayload> = (state, { action, payload }) => {
    switch (action) {
        case DispatchActionKey.SetConfigs:
            return {
                ...state,
                configs: typeof payload === 'function' ? payload(state.configs) : payload,
            };
        case DispatchActionKey.SetSort:
            return {
                ...state,
                sort: typeof payload === 'function' ? payload(state.sort) : payload,
            };
        case DispatchActionKey.SetCheckForUpdates:
            return {
                ...state,
                checkForUpdates: typeof payload === 'function' ? payload(state.checkForUpdates) : payload,
            };
        case DispatchActionKey.LoadConfig:
            return {
                ...state,
                ...((typeof payload === 'function' ? payload(state) : payload) as Partial<AppState>),
            };
        case DispatchActionKey.SaveConfig:
            saveState(state);
            return state;
        case DispatchActionKey.LaunchConfig:
            invoke('launch_config', { config: (payload as LauncherConfigPayload).config })
                .catch((err) => {
                    console.error(err);
                })
                .finally(() => {
                    (payload as LauncherConfigPayload).onComplete?.call(this);
                });
            return state;
        default:
            throw new Error('Unknown action type.');
    }
};

export const AppReducer: [Reducer<AppState, DispatchPayload>, AppState] = [reducer, initialState];

export interface ContextValue {
    state: AppState;
    dispatch: Dispatch<DispatchPayload<unknown>>;
}

export default createContext<ContextValue>({ state: initialState, dispatch: () => initialState });
