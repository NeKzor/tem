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

export interface LauncherMod {
    name: string;
    version: string;
}

export interface LauncherMods {
    tem: LauncherMod;
    xdead: LauncherMod;
}

export type SortOption = {
    key: keyof LauncherConfig;
    direction: 'asc' | 'desc';
};

export type SortOrderOption = 'createdAt-asc' | 'createdAt-desc' | 'name-asc' | 'name-desc';
export type CheckForUpdatesOption = 'disabled' | 'on-launcher-start' | 'on-launcher-exit';

export interface AppConfig {
    configs: Partial<LauncherConfig>[];
    sort: SortOption;
    checkForUpdates: CheckForUpdatesOption;
}
