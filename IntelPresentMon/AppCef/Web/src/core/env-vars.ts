import { Api } from "./api";

export interface EnvVars {
    useDebugBlocklist: boolean;
};

var vars:EnvVars|null = null;

export async function GetEnvVars(): Promise<EnvVars> {
    if (vars === null) {
        vars = await Api.loadEnvVars()
    }
    return vars;
}