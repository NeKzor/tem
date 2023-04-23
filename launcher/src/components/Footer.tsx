import { Typography } from '@material-tailwind/react';

type FooterLink = {
    label: string;
    url: string;
};

const links: FooterLink[] = [
    {
        label: 'Documentation',
        url: 'https://tem.nekz.me',
    },
    {
        label: 'Repository',
        url: 'https://github.com/NeKzor/tem',
    },
    {
        label: 'Issues',
        url: 'https://github.com/NeKzor/tem/issues',
    },
    {
        label: 'Discussions',
        url: 'https://github.com/NeKzor/tem/discussions',
    },
    {
        label: 'Project',
        url: 'https://github.com/users/NeKzor/projects/1/views/1',
    },
];

function Footer() {
    return (
        <footer className="fixed bottom-0 w-full bg-white p-8 pt-0 z-50">
            <hr className="mb-8 border-blue-gray-50" />
            <div className="flex flex-row flex-wrap items-center justify-center gap-y-6 gap-x-12 bg-white text-center md:justify-between">
                <Typography color="blue-gray" className="text-center font-normal">
                    &copy; 2023 TEM
                </Typography>
                <ul className="flex flex-wrap items-center gap-y-2 gap-x-8">
                    {links.map((link) => {
                        return (
                            <li>
                                <Typography
                                    as="a"
                                    href={link.url}
                                    title={link.url}
                                    target="_blank"
                                    rel="noopener noreferrer"
                                    color="blue-gray"
                                    className="font-normal transition-colors hover:text-blue-500 focus:text-blue-500"
                                >
                                    {link.label}
                                </Typography>
                            </li>
                        );
                    })}
                </ul>
            </div>
        </footer>
    );
}

export default Footer;
