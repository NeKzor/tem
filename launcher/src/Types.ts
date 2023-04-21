export interface LauncherConfig {
    name: string;
    createdAt: number;
    modifiedAt: number;
    windowWidth: number;
    windowHeight: number;
    isFullscreen: boolean;
    disableSplashScreen: boolean;
    isDefault: boolean;
    useTEM: boolean;
    useXDead: boolean;
}

export const createLauncherConfig = (data?: Partial<LauncherConfig>): LauncherConfig => {
    return Object.assign<LauncherConfig, Partial<LauncherConfig> | undefined>(
        {
            name: '',
            createdAt: 0,
            modifiedAt: 0,
            windowWidth: window.screen.width,
            windowHeight: window.screen.height,
            isFullscreen: true,
            disableSplashScreen: true,
            isDefault: false,
            useTEM: true,
            useXDead: false,
        },
        data,
    );
};

export interface LauncherMod {
    name: string;
    version: string;
}

export interface LauncherMods {
    tem: LauncherMod;
    xdead: LauncherMod;
}

export const defaultMods: LauncherMods = {
    tem: {
        name: 'TEM',
        version: 'unknown version',
    },
    xdead: {
        name: 'XDead',
        version: 'unknown version',
    },
};

export type SortOption = {
    key: keyof LauncherConfig;
    direction: 'asc' | 'desc';
};

export type SortOrderOption = 'createdAt-asc' | 'createdAt-desc' | 'name-asc' | 'name-desc';
export type CheckForUpdatesOption = 'disabled' | 'on-launcher-start' | 'on-launcher-exit';
