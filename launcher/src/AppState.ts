import { Reducer, createContext } from 'react';
import { CheckForUpdatesOption, LauncherConfig, SortOption } from './Types';

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

export enum DispatchAction {
    SetCheckForUpdates = 'SetCheckForUpdates',
    SaveConfig = 'SaveConfig',
}

export interface DispatchPayload<T = unknown> {
    action: DispatchAction,
    payload: T;
}

const reducer: Reducer<AppState, DispatchPayload> = (state, { action, payload }) => {
    switch (action) {
        case DispatchAction.SetCheckForUpdates:
            return {
                ...state,
            };
        case DispatchAction.SaveConfig:
            throw new Error('Action type not implemented.');
        default:
            throw new Error('Unknown action type.');
    }
};

export const AppReducer: [Reducer<AppState, DispatchPayload>, AppState] = [reducer, initialState];

export interface ContextValue {
    state: AppState;
    dispatch: (dispatchPayload: DispatchPayload) => AppState,
}

export default createContext<ContextValue>({ state: initialState, dispatch: () => initialState });
